/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This is an example plugin to show how to render something inside
 * of the transparent cube
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
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
 * Based on atlatins xscreensaver http://www.jwz.org/xscreensaver/
 */

/* atlantis --- Shows moving 3D sea animals */

/* Copyright (c) E. Lassauge, 1998. */

/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * The original code for this mode was written by Mark J. Kilgard
 * as a demo for openGL programming.
 *
 * Porting it to xlock  was possible by comparing the original Mesa's morph3d
 * demo with it's ported version to xlock, so thanks for Marcelo F. Vianna
 * (look at morph3d.c) for his indirect help.
 *
 * Thanks goes also to Brian Paul for making it possible and inexpensive
 * to use OpenGL at home.
 *
 * My e-mail address is lassauge@users.sourceforge.net
 *
 * Eric Lassauge  (May-13-1998)
 *
 */

/**
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
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>



#include "atlantis-internal.h"
#include "atlantis_options.h"

static int displayPrivateIndex;

static int cubeDisplayPrivateIndex;

typedef struct _AtlantisDisplay
{
    int screenPrivateIndex;

}

AtlantisDisplay;


#define GET_ATLANTIS_DISPLAY(d) \
    ((AtlantisDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define ATLANTIS_DISPLAY(d) \
    AtlantisDisplay *ad = GET_ATLANTIS_DISPLAY(d);

#define GET_ATLANTIS_SCREEN(s, ad) \
    ((AtlantisScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)
#define ATLANTIS_SCREEN(s) \
    AtlantisScreen *as = GET_ATLANTIS_SCREEN(s, GET_ATLANTIS_DISPLAY(s->display))

static void
initAtlantis (CompScreen *s)
{
    ATLANTIS_SCREEN (s);
    int         i;

    as->numFish = atlantisGetNumFish (s);
    as->fish = calloc (as->numFish, sizeof (fishRec) );

    for (i = 0; i < as->numFish; i++)
    {
	as->fish[i].type = NRAND (20);
	as->fish[i].type = MIN ( 3, as->fish[i].type);

	switch (as->fish[i].type)
	{

	case SHARK:
	    as->fish[i].size = NRAND (atlantisGetSharkSize (s) ) +
			       atlantisGetSharkSize (s);

	    as->fish[i].speed = NRAND (150) + 50;
	    as->fish[i].color[0] = ( (float) NRAND (100) / 200.0) + 0.25;
	    as->fish[i].color[1] = as->fish[i].color[0];
	    as->fish[i].color[2] = as->fish[i].color[0];
	    break;

	case WHALE:
	    as->fish[i].size = NRAND (atlantisGetWhaleSize (s) ) +
			       atlantisGetWhaleSize (s);
	    as->fish[i].size /= 4;

	    as->fish[i].speed = NRAND (100) + 50;
	    as->fish[i].color[0] = ( (float) NRAND (100) / 500.0) + 0.80;
	    as->fish[i].color[1] = as->fish[i].color[0];
	    as->fish[i].color[2] = as->fish[i].color[0];
	    break;

	case DOLPHIN:
	    as->fish[i].size = NRAND (atlantisGetDolphinSize (s) ) +
			       atlantisGetDolphinSize (s);

	    as->fish[i].speed = NRAND (150) + 50;
	    as->fish[i].color[0] = ( (float) NRAND (100) / 200.0) + 0.50;
	    as->fish[i].color[1] = as->fish[i].color[0];
	    as->fish[i].color[2] = as->fish[i].color[0];
	    break;

	case FISH:
	    as->fish[i].size = NRAND (atlantisGetFishSize (s) ) +
			       atlantisGetFishSize (s);

	    as->fish[i].speed = NRAND (150) + 50;

	    if (NRAND (10) < 7)
	    {
		as->fish[i].color[0] = ( (float) NRAND (100) / 200.0) + 0.50;
		as->fish[i].color[1] = as->fish[i].color[0];
		as->fish[i].color[2] = as->fish[i].color[0];
	    }
	    else
	    {
		as->fish[i].color[0] = ( (float) NRAND (100) / 100.0);
		as->fish[i].color[1] = ( (float) NRAND (100) / 100.0);
		as->fish[i].color[2] = ( (float) NRAND (100) / 100.0);
	    }

	    break;

	default:
	    break;
	}

	as->fish[i].x = NRAND (as->fish[i].size);
	as->fish[i].y = NRAND (as->fish[i].size);
	as->fish[i].z = NRAND (as->fish[i].size / 100);
	as->fish[i].xt = NRAND (30000) - 15000;
	as->fish[i].yt = NRAND (30000) - 15000;
	as->fish[i].zt = NRAND (30000) - 15000;
	as->fish[i].psi = NRAND (360) - 180.0;
	as->fish[i].v = 1.0;
	as->fish[i].sign = (NRAND (2) == 0) ? 1 : -1;
    }
}

static void
freeAtlantis (CompScreen *s)
{
    ATLANTIS_SCREEN (s);

    if (as->fish)
	free (as->fish);

    as->fish = NULL;
}

static void
updateAtlantis (CompScreen *s)
{
    freeAtlantis (s);
    initAtlantis (s);
}

static void
atlantisScreenOptionChange (CompScreen            *s,
			    CompOption            *opt,
			    AtlantisScreenOptions num)
{
    updateAtlantis (s);
}

static void
atlantisClearTargetOutput (CompScreen *s,
			   float      xRotate,
			   float      vRotate)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (as, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, vRotate);
    WRAP (as, cs, clearTargetOutput, atlantisClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static void atlantisPaintInside (CompScreen              *s,
				 const ScreenPaintAttrib *sAttrib,
				 const CompTransform     *transform,
				 CompOutput              *output,
				 int                     size)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int i;
    float scale;

    static const float mat_shininess[]      = { 90.0 };
    static const float mat_specular[]       = { 0.8, 0.8, 0.8, 1.0 };
    static const float mat_diffuse[]        = { 0.46, 0.66, 0.795, 1.0 };
    static const float mat_ambient[]        = { 0.0, 0.1, 0.2, 1.0 };
    static const float lmodel_ambient[]     = { 0.4, 0.4, 0.4, 1.0 };
    static const float lmodel_localviewer[] = { 0.0 };

    ScreenPaintAttrib sA = *sAttrib;
    CompTransform mT = *transform;

    sA.yRotate += (360.0f / size) * (cs->xRotations - (s->x * cs->nOutput));

    (*s->applyScreenTransform) (s, &sA, output, &mT);

    glPushMatrix();

    glLoadMatrixf (mT.m);

    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);

    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    Bool enabledCull = FALSE;

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

    glEnable (GL_BLEND);

    if (glIsEnabled (GL_CULL_FACE))
    {
	enabledCull = TRUE;
	glEnable (GL_CULL_FACE);
    }


    glPushMatrix();

    glScalef (0.00001, 0.00001, 0.00001);
    glColor4usv (defaultColor);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);

    glEnable (GL_NORMALIZE);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT1);
    glEnable (GL_LIGHT0);

    if (atlantisGetColors (s))
	glEnable (GL_COLOR_MATERIAL);
    else
	glDisable (GL_COLOR_MATERIAL);

    glEnable (GL_DEPTH_TEST);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    for (i = 0; i < as->numFish; i++)
    {
	glPushMatrix();
	FishTransform (& (as->fish[i]) );
	scale = as->fish[i].size;
	scale /= 6000.0;
	glScalef (scale, scale, scale);
	glColor3fv (as->fish[i].color);

	switch (as->fish[i].type)
	{
	case SHARK:
	    DrawShark (& (as->fish[i]), 0);
	    break;

	case WHALE:
	    DrawWhale (& (as->fish[i]), 0);
	    break;

	case DOLPHIN:
	    DrawDolphin (& (as->fish[i]), 0);
	    break;

	case FISH:
	    DrawDolphin (& (as->fish[i]), 0);
	    break;

	default:
	    break;
	}

	glPopMatrix();
    }

    glPopMatrix();

    glDisable (GL_LIGHT1);
    glDisable (GL_NORMALIZE);

    if (!s->lighting)
	glDisable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

    glPopMatrix();

    glPopAttrib();

    as->damage = TRUE;

    glColor4usv (defaultColor);

    UNWRAP (as, cs, paintInside);
    (*cs->paintInside) (s, sAttrib, transform, output, size);
    WRAP (as, cs, paintInside, atlantisPaintInside);
}

static void
atlantisPreparePaintScreen (CompScreen *s,
			    int        ms)
{
    ATLANTIS_SCREEN (s);

    int         i;

    for (i = 0; i < as->numFish; i++)
    {
	FishPilot (& (as->fish[i]), as->fish[i].speed);
	FishMiss (as, i);
    }

    UNWRAP (as, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (as, s, preparePaintScreen, atlantisPreparePaintScreen);
}

static void
atlantisDonePaintScreen (CompScreen * s)
{
    ATLANTIS_SCREEN (s);

    if (as->damage)
    {
	damageScreen (s);
	as->damage = FALSE;
    }

    UNWRAP (as, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (as, s, donePaintScreen, atlantisDonePaintScreen);
}


static Bool
atlantisInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    AtlantisDisplay *ad;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
	!checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    ad = malloc (sizeof (AtlantisDisplay));

    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = ad;

    return TRUE;
}

static void
atlantisFiniDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    ATLANTIS_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);
    free (ad);
}

static Bool
atlantisInitScreen (CompPlugin *p,
		    CompScreen *s)
{
    static const float ambient[]  = { 0.1, 0.1, 0.1, 1.0 };
    static const float diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
    static const float position[] = { 0.0, 1.0, 0.0, 0.0 };

    AtlantisScreen *as;
    
    ATLANTIS_DISPLAY (s->display);
    CUBE_SCREEN (s);

    as = malloc (sizeof (AtlantisScreen) );

    if (!as)
	return FALSE;

    s->base.privates[ad->screenPrivateIndex].ptr = as;

    as->damage = FALSE;

    glLightfv (GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT1, GL_POSITION, position);

    initAtlantis (s);

    atlantisSetNumFishNotify (s, atlantisScreenOptionChange);
    atlantisSetSharkSizeNotify (s, atlantisScreenOptionChange);
    atlantisSetWhaleSizeNotify (s, atlantisScreenOptionChange);
    atlantisSetDolphinSizeNotify (s, atlantisScreenOptionChange);
    atlantisSetFishSizeNotify (s, atlantisScreenOptionChange);

    WRAP (as, s, donePaintScreen, atlantisDonePaintScreen);
    WRAP (as, s, preparePaintScreen, atlantisPreparePaintScreen);
    WRAP (as, cs, clearTargetOutput, atlantisClearTargetOutput);
    WRAP (as, cs, paintInside, atlantisPaintInside);

    return TRUE;
}

static void
atlantisFiniScreen (CompPlugin *p,
		    CompScreen *s)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    freeAtlantis (s);

    UNWRAP (as, s, donePaintScreen);
    UNWRAP (as, s, preparePaintScreen);
    UNWRAP (as, cs, clearTargetOutput);
    UNWRAP (as, cs, paintInside);

    free (as);
}

static Bool
atlantisInit (CompPlugin * p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
atlantisFini (CompPlugin * p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
atlantisInitObject (CompPlugin *p,
		 CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) atlantisInitDisplay,
	(InitPluginObjectProc) atlantisInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
atlantisFiniObject (CompPlugin *p,
		 CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) atlantisFiniDisplay,
	(FiniPluginObjectProc) atlantisFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable atlantisVTable = {

    "atlantis",
    0,
    atlantisInit,
    atlantisFini,
    atlantisInitObject,
    atlantisFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &atlantisVTable;
}

