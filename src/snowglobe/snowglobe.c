/*
 * Compiz cube snowglobe plugin
 *
 * snowglobe.c
 *
 * This is a test plugin to show falling snow inside
 * of the transparent cube
 *
 * Written in 2007 by David Mikos
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Based on atlantis and snow plugins
 */

/**
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>



#include "snowglobe-internal.h"
#include "snowglobe_options.h"

static Bool snowglobeIsCylinder(CompScreen *);

int snowglobeDisplayPrivateIndex;

int cubeDisplayPrivateIndex;



static void
initSnowglobe (CompScreen *s)
{

    SNOWGLOBE_SCREEN (s);

    as->water = NULL;
    as->ground = NULL;

    as->numSnowflakes = snowglobeGetNumSnowflakes(s);

    as->snow = calloc(as->numSnowflakes, sizeof(snowflakeRec));

    initializeWorldVariables(s);

    int i;
    for (i=0; i< as->numSnowflakes; i++)
    {
	as->snow[i].size = snowglobeGetSnowflakeSize(s)
		+sqrt(randf(snowglobeGetSnowflakeSize(s)));

	newSnowflakePosition(as, i);

	as->snow[i].psi = randf(2*PI);
	as->snow[i].theta= randf(PI);

	as->snow[i].dpsi = randf(5);
	as->snow[i].dtheta = randf(5);

	as->snow[i].speed = randf(0.4)+0.2;

    }

    as->waterHeight = 50000;

    as->oldProgress = 0;

   
    as->snowflakeDisplayList = glGenLists(1);
    glNewList(as->snowflakeDisplayList, GL_COMPILE);
    DrawSnowflake(0);
    glEndList();
}

void
initializeWorldVariables(CompScreen *s)
{
    SNOWGLOBE_SCREEN (s);
    CUBE_SCREEN (s);

    as->speedFactor = snowglobeGetSpeedFactor(s);

    as->hsize = s->hsize * cs->nOutput;

    as->arcAngle = 360.0f / as->hsize;
    as->radius = cs->distance/sinf(0.5*(PI-as->arcAngle*toRadians));
    as->distance = cs->distance;
}

static void
freeSnowglobe (CompScreen *s)
{
    SNOWGLOBE_SCREEN (s);

    if (as->snow)
	free (as->snow);

    freeWater (as->water);
    freeWater (as->ground);

    glDeleteLists(as->snowflakeDisplayList,  1);
}

static void
updateSnowglobe (CompScreen *s)
{
    freeSnowglobe (s);
    initSnowglobe (s);
}
static void
snowglobeScreenOptionChange (CompScreen *s,
		CompOption            *opt,
		SnowglobeScreenOptions num)
{
    updateSnowglobe (s);
}
static void
snowglobeSpeedFactorOptionChange (CompScreen *s,
		CompOption            *opt,
		SnowglobeScreenOptions num)
{
    SNOWGLOBE_SCREEN (s);
    as->speedFactor = snowglobeGetSpeedFactor(s);
}

static void
snowglobeClearTargetOutput (CompScreen *s,
		float      xRotate,
		float      vRotate)
{
    SNOWGLOBE_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (as, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, vRotate);
    WRAP (as, cs, clearTargetOutput, snowglobeClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static Bool
snowglobeIsCylinder(CompScreen *s)
{
    CUBE_SCREEN (s);
    
    static const int CYLINDER = 1;
   
    CompPlugin *p = NULL;
    const char plugin[] = "cubeaddon";
    p = findActivePlugin (plugin);
    if (p && p->vTable->getObjectOptions)
    {
	CompOption *option;
	int  nOption;
	Bool cylinderManualOnly = FALSE;
	Bool unfoldDeformation = TRUE;
	
	option = (*p->vTable->getObjectOptions) (p, (CompObject *)s,
		&nOption);
	option = compFindOption (option, nOption, "deformation", 0);

	if (option)
	    if (option->value.b)
		cylinderManualOnly = option->value.b;

	option = (*p->vTable->getObjectOptions) (p, (CompObject *)s,
		&nOption);
	option = compFindOption (option, nOption, "cylinder_manual_only", 0);
	
	if (option)
	    if (option->value.b)
		unfoldDeformation = option->value.b;
	
	if (s->hsize * cs->nOutput > 2 && s->desktopWindowCount &&
	    (cs->rotationState == RotationManual ||
	    (cs->rotationState == RotationChange &&
	    !cylinderManualOnly)) &&
	    (!cs->unfolded || unfoldDeformation))
	{
	    option = (*p->vTable->getObjectOptions) (p, (CompObject *)s,
		      &nOption);
	    option = compFindOption (option, nOption, "deformation", 0);

	    if (option)
		return (option->value.i==CYLINDER);
	}
    }
    return FALSE;
}




static void
snowglobePaintInside (CompScreen *s,
		const ScreenPaintAttrib *sAttrib,
		const CompTransform     *transform,
		CompOutput              *output,
		int                     size)
{
    SNOWGLOBE_SCREEN (s);
    CUBE_SCREEN (s);

    int i;

    as->waterHeight = 50000;

    if (as->hsize!=s->hsize) updateSnowglobe (s);


    static const float mat_shininess[] = { 60.0 };
    static const float mat_specular[] = { 0.8, 0.8, 0.8, 1.0 };
    static const float mat_diffuse[] = { 0.46, 0.66, 0.795, 1.0 };
    static const float mat_ambient[] = { 0.1, 0.1, 0.3, 1.0 };
    static const float lmodel_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float lmodel_localviewer[] = { 0.0 };

    ScreenPaintAttrib sA = *sAttrib;
    CompTransform mT = *transform;

    float progress, dummy;
    (*cs->getRotation) (s, &dummy, &dummy, &progress);
    
    if (snowglobeGetShowWater(s))
	updateHeight(as->water);
    {
	Bool cylinder = snowglobeIsCylinder(s);
	Bool deform = FALSE;
	
	if (cylinder)
	{
	    if (fabsf(1.0f - progress) < floatErr)
		progress = 1.0f;
	    
	    if (as->oldProgress != 1.0f || progress != 1.0f)
	    {
		deform = TRUE;
		as->oldProgress = progress;
	    }
	}
	else if (as->oldProgress != 0.0f)
	{
	    if (fabsf(progress) < floatErr)
		progress = 0.0f;

	    deform = TRUE;

	    as->oldProgress = progress;
	}	
	    
	if (deform)
	{
	    if (snowglobeGetShowWater (s))
		deformCylinder(s, as->water, progress);

	    if (atlantisGetShowGround (s))
	    {
		deformCylinder(s, as->ground, progress);
		updateHeight (as->ground, FALSE);
	    }
	}
	
    sA.yRotate += cs->invert * (360.0f / size) *
		 (cs->xRotations - (s->x* cs->nOutput));

    (*s->applyScreenTransform)(s, &sA, output, &mT);

    glPushMatrix();

    glLoadMatrixf(mT.m);

    glTranslatef(cs->outputXOffset, -cs->outputYOffset, 0.0f);

    glScalef(cs->outputXScale, cs->outputYScale, 1.0f);

    Bool enabledCull = FALSE;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

    glEnable(GL_BLEND);

    if (glIsEnabled(GL_CULL_FACE))
    {
	enabledCull = TRUE;
    }

    int cull;

    glGetIntegerv(GL_CULL_FACE_MODE, &cull);
    glEnable(GL_CULL_FACE);

    glCullFace(~cull & (GL_FRONT | GL_BACK));

    if (snowglobeGetShowWater(s))
    {
	glColor4usv(snowglobeGetWaterColor(s));
	drawWater(as->water, TRUE, FALSE);
    }
    glCullFace(cull);

    if (snowglobeGetShowGround (s) && !snowglobeIsCylinder(s))
    {
	glColor4f(0.8, 0.8, 0.8, 1.0);
	drawGround(NULL, as->ground);

    }

    glPushMatrix();

    glColor4usv(defaultColor);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);

    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    for (i = 0; i < as->numSnowflakes; i++)
    {
	glPushMatrix();
	SnowflakeTransform(&(as->snow[i]));

	float scale = 0.01*as->snow[i].size;
	glScalef(scale, scale, scale);

	initDrawSnowflake();
	glCallList(as->snowflakeDisplayList);
	finDrawSnowflake();
	glPopMatrix();
    }

    if (snowglobeGetShowSnowman(s))
    {
	glPushMatrix();

	float bottom = -0.5;
	if (snowglobeGetShowGround(s))
	    bottom = getHeight(as->ground, 0, 0);
	glTranslatef(0, bottom, 0);

	float scale = 0.4*snowglobeGetSnowmanSize(s)*(0.5-bottom);
	glScalef(scale, scale, scale);

	glColor4f(1.0, 1.0, 1.0, 1.0);

	DrawSnowman(0);
	glPopMatrix();
    }

    glPopMatrix();

    if (snowglobeGetShowWater(s))
    {
	glEnable(GL_CULL_FACE);
	glColor4usv(snowglobeGetWaterColor(s));
	drawWater(as->water, snowglobeGetShowWater(s), 0);
    }

    if (snowglobeGetShowGround(s))
    {
	glColor4f(0.8, 0.8, 0.8, 1.0);
	drawBottomGround(s->hsize * cs->nOutput, cs->distance, -0.4999);
    }

    glDisable(GL_LIGHT1);
    glDisable(GL_NORMALIZE);

    if (!s->lighting)
	glDisable(GL_LIGHTING);

    glDisable(GL_DEPTH_TEST);

    if (enabledCull)
	glDisable(GL_CULL_FACE);

    glPopMatrix();

    glPopAttrib();

    as->damage = TRUE;

    UNWRAP (as, cs, paintInside);
    (*cs->paintInside)(s, sAttrib, transform, output, size);
    WRAP (as, cs, paintInside, snowglobePaintInside);
}

static void
snowglobePreparePaintScreen (CompScreen *s,
		int        ms)
{
    SNOWGLOBE_SCREEN (s);

    int i;

    Bool cylinder = snowglobeIsCylinder(s);
    int oldhsize = as->hsize;
    
    updateWater (s, (float)ms / 1000.0);
    updateGround (s, (float)ms / 1000.0);
	
    if (cylinder && as->oldProgress>0.9)
    {
	as->hsize*=32/as->hsize;
	as->arcAngle = 360.0f / as->hsize;
	as->sideDistance = as->radius * as->ratio;
    }
	
    for (i = 0; i < as->numSnowflakes; i++)
    {
	SnowflakeDrift(s, i);
    }

    updateWater(s, (float)ms / 1000.0);
    updateGround(s, (float)ms / 1000.0);
    
    as->hsize = oldhsize;
    as->arcAngle = 360.0f / as->hsize;
    as->sideDistance = as->topDistance * as->ratio;
	
    UNWRAP (as, s, preparePaintScreen);
    (*s->preparePaintScreen)(s, ms);
    WRAP (as, s, preparePaintScreen, snowglobePreparePaintScreen);
}

static void
snowglobeDonePaintScreen (CompScreen * s)
{
    SNOWGLOBE_SCREEN (s);

    if (as->damage)
    {
	damageScreen (s);
	as->damage = FALSE;
    }

    UNWRAP (as, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (as, s, donePaintScreen, snowglobeDonePaintScreen);
}

static Bool
snowglobeInitDisplay (CompPlugin  *p,
		CompDisplay *d)
{
    SnowglobeDisplay *ad;

    if (!checkPluginABI("core", CORE_ABIVERSION)
	    || !checkPluginABI("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex(d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    ad = malloc(sizeof(SnowglobeDisplay));

    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex(d);

    if (ad->screenPrivateIndex < 0)
    {
	free(ad);
	return FALSE;
    }

    d->base.privates[snowglobeDisplayPrivateIndex].ptr = ad;

    return TRUE;
}

static void
snowglobeFiniDisplay (CompPlugin  *p,
		CompDisplay *d)
{
    SNOWGLOBE_DISPLAY (d);

    freeScreenPrivateIndex(d, ad->screenPrivateIndex);
    free(ad);
}

static Bool
snowglobeInitScreen (CompPlugin *p,
		CompScreen *s)
{
    static const float ambient[] = { 0.3, 0.3, 0.3, 1.0 };
    static const float diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float position[] = { 0.0, 1.0, 0.0, 0.0 };

    SnowglobeScreen *as;

    SNOWGLOBE_DISPLAY (s->display);
    CUBE_SCREEN (s);

    as = malloc(sizeof(SnowglobeScreen));

    if (!as)
	return FALSE;

    s->base.privates[ad->screenPrivateIndex].ptr = as;

    as->damage = FALSE;

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, position);

    initSnowglobe(s);

    snowglobeSetSpeedFactorNotify(s, snowglobeSpeedFactorOptionChange);

    snowglobeSetNumSnowflakesNotify(s, snowglobeScreenOptionChange);
    snowglobeSetSnowflakeSizeNotify(s, snowglobeScreenOptionChange);

    WRAP (as, s, donePaintScreen, snowglobeDonePaintScreen);
    WRAP (as, s, preparePaintScreen, snowglobePreparePaintScreen);
    WRAP (as, cs, clearTargetOutput, snowglobeClearTargetOutput);
    WRAP (as, cs, paintInside, snowglobePaintInside);

    return TRUE;
}

static void
snowglobeFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    SNOWGLOBE_SCREEN (s);
    CUBE_SCREEN (s);

    freeSnowglobe(s);

    UNWRAP (as, s, donePaintScreen);
    UNWRAP (as, s, preparePaintScreen);
    UNWRAP (as, cs, clearTargetOutput);
    UNWRAP (as, cs, paintInside);

    free(as);
}

static Bool
snowglobeInit (CompPlugin * p)
{
    snowglobeDisplayPrivateIndex = allocateDisplayPrivateIndex();

    if (snowglobeDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
snowglobeFini (CompPlugin * p)
{
    if (snowglobeDisplayPrivateIndex >= 0)
	freeDisplayPrivateIndex (snowglobeDisplayPrivateIndex);
}

static CompBool
snowglobeInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0, /* InitCore */
		(InitPluginObjectProc) snowglobeInitDisplay,
		(InitPluginObjectProc) snowglobeInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
snowglobeFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
		(FiniPluginObjectProc) 0, /* FiniCore */
		(FiniPluginObjectProc) snowglobeFiniDisplay,
		(FiniPluginObjectProc) snowglobeFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable snowglobeVTable = {

		"snowglobe",
		0,
		snowglobeInit,
		snowglobeFini,
		snowglobeInitObject,
		snowglobeFiniObject,
		0,
		0
};


CompPluginVTable *
getCompPluginInfo (void)
{
    return &snowglobeVTable;
}
