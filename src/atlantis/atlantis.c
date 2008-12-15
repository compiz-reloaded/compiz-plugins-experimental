/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This plugin renders a fish tank inside of the transparent cube,
 * replete with fish, crabs, sand, bubbles, and coral.
 *
 * Copyright : (C) 2007-2008 by David Mikos
 * Email     : infiniteloopcounter@gmail.com
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
 * Detailed fish/fish2 and coral 3D models made by Biswajyoti Mahanta.
 *
 * Butterflyfish and Chromis 3D models/auto-generated code by "unpush"
 */

/*
 * Based on atlantis xscreensaver http://www.jwz.org/xscreensaver/
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


/*
 * Coordinate system:
 * clockwise from top
 * with x=radius, y=0 the "top" (towards 1st desktop from above view)
 * and the z coordinate as height.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "atlantis-internal.h"
#include "atlantis_options.h"


static void initWorldVariables (CompScreen *);
static void loadModels (CompScreen *);
static void freeModels (CompScreen *s);
static void setLightPosition (CompScreen *, GLenum light);
static void atlantisInitLightPosition (CompScreen *);

int atlantisDisplayPrivateIndex;

int cubeDisplayPrivateIndex;


static void
initAtlantis (CompScreen *s)
{
    ATLANTIS_SCREEN (s);

    int i = 0, i2 = 0;
    int j, k;
    int num;

    CompListValue * cType   = atlantisGetCreatureType (s);
    CompListValue * cNumber = atlantisGetCreatureNumber (s);
    CompListValue * cSize   = atlantisGetCreatureSize (s);
    CompListValue * cColor  = atlantisGetCreatureColor (s);

    num = MIN (cType->nValue, cNumber->nValue);
    num = MIN (num, cSize->nValue);
    num = MIN (num, cColor->nValue);

    as->water = NULL;
    as->ground = NULL;

    as->numFish = 0;
    as->numCrabs = 0;

    for (k = 0; k < num; k++)
    {
	if (cSize->value[k].i == 0)
	    continue;

	if (cType->value[k].i != CRAB)
	    as->numFish += cNumber->value[k].i;
	else
	    as->numCrabs += cNumber->value[k].i;
    }

    as->fish = calloc (as->numFish,  sizeof(fishRec));
    as->crab = calloc (as->numCrabs, sizeof(crabRec));

    if (atlantisGetShowWater (s))
	as->waterHeight = atlantisGetWaterHeight (s) * 100000 - 50000;
    else
	as->waterHeight = 50000;

    as->oldProgress = 0;

    for (k = 0; k < num; k++)
    {
	for (j = 0; j < cNumber->value[k].i; j++)
	{
	    int size = cSize->value[k].i;
	    int type = cType->value[k].i;

	    if (size==0)
		break;

	    if (type != CRAB)
	    {
		fishRec * fish = &(as->fish[i]);

		fish->type = type;

		if (type == WHALE)
		    size /= 2;
		if (type == DOLPHIN)
		    size *= 2;
		if (type == SHARK)
		    size *= 3;

		fish->size = randf (sqrtf (size) ) + size;
		fish->speed = randf (150) + 50;

		if (j == 0)
		    setSimilarColor4us (fish->color, cColor->value[k].c,
		                        0.2, 0.1);
		else
		    setSimilarColor (fish->color, as->fish[i-j].color,
		                     0.2, 0.1);

		fish->x = randf (size);
		fish->y = randf (size);
		fish->z = (as->waterHeight - 50000) / 2 +
			  randf (size * 0.02) - size * 0.01;
		fish->psi = randf (360) - 180.0;
		fish->theta = randf (100) - 50;
		fish->v = 1.0;

		fish->group = k;

		fish->boidsCounter = i % NUM_GROUPS;
		fish->boidsPsi = fish->psi;
		fish->boidsTheta = fish->theta;

		fish->smoothTurnCounter = NRAND (3);
		fish->smoothTurnAmount = NRAND (3) - 1;

		fish->prevRandPsi = 0;
		fish->prevRandTh = 0;

		i++;
	    }
	    else
	    {
		crabRec * crab = &(as->crab[i2]);

		crab->size = randf (sqrtf (size)) + size;
		crab->speed = randf (100) + 50;

		if (j == 0)
		    setSimilarColor4us (crab->color, cColor->value[k].c,
		                        0.2, 0.1);
		else
		    setSimilarColor (crab->color, as->crab[i2 - j].color,
		                     0.2, 0.1);

		crab->x = randf (2 * size) - size;
		crab->y = randf (2 * size) - size;

		if (atlantisGetStartCrabsBottom (s))
		{
		    crab->z = 50000;
		    crab->isFalling = FALSE;
		}
		else
		{
		    crab->z = (as->waterHeight - 50000)/2;
		    crab->isFalling = TRUE;
		}

		crab->psi = randf (360);
		crab->theta= 0;

		crab->scuttlePsi = 0;
		crab->scuttleAmount = NRAND (3) - 1;

		i2++;
	    }
	}
    }

    as->numCorals = 0;
    as->numAerators = 0;

    cType = atlantisGetPlantType (s);
    cNumber = atlantisGetPlantNumber (s);
    cSize = atlantisGetPlantSize (s);
    cColor = atlantisGetPlantColor (s);

    num = MIN (cType->nValue, cNumber->nValue);
    num = MIN (num, cSize->nValue);
    num = MIN (num, cColor->nValue);

    for (k = 0; k < num; k++)
    {
	switch (cType->value[k].i) {
	case 0:
	case 1:
	    as->numCorals += cNumber->value[k].i;
	    break;

	case 2:
	    as->numAerators += cNumber->value[k].i;
	    break;
	}
    }

    as->coral   = calloc (as->numCorals,   sizeof(coralRec));
    as->aerator = calloc (as->numAerators, sizeof(aeratorRec));

    for (k = 0; k < as->numAerators; k++)
    {
	as->aerator[k].numBubbles = 20;
	as->aerator[k].bubbles = calloc (as->aerator[k].numBubbles,
		sizeof (Bubble));
    }

    initWorldVariables(s);

    updateWater (s, 0); /* make sure normals are initialized */
    updateGround (s, 0);

    loadModels(s);
}

static void
loadModels (CompScreen *s)
{
    ATLANTIS_SCREEN (s);

    as->crabDisplayList = glGenLists (1);
    glNewList (as->crabDisplayList, GL_COMPILE);
    DrawCrab (0);
    glEndList ();

    as->coralDisplayList = glGenLists (1);
    glNewList (as->coralDisplayList, GL_COMPILE);
    atlantisGetLowPoly (s) ? DrawCoralLow (0) : DrawCoral (0);
    glEndList ();

    as->coral2DisplayList = glGenLists (1);
    glNewList (as->coral2DisplayList, GL_COMPILE);
    atlantisGetLowPoly (s) ? DrawCoral2Low (0) : DrawCoral2 (0);
    glEndList ();

    as->bubbleDisplayList = glGenLists (1);
    glNewList (as->bubbleDisplayList, GL_COMPILE);
    atlantisGetLowPoly (s) ? DrawBubble (0, 6) : DrawBubble (0, 9);
    glEndList ();
}

static void
freeModels (CompScreen *s)
{
    ATLANTIS_SCREEN (s);

    glDeleteLists (as->crabDisplayList, 1);
    glDeleteLists (as->coralDisplayList, 1);
    glDeleteLists (as->coral2DisplayList, 1);
    glDeleteLists (as->bubbleDisplayList, 1);
}

static void
initWorldVariables (CompScreen *s)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int i = 0, i2 = 0;
    int j, k;
    int bi, num;

    coralRec * coral;
    aeratorRec * aerator;

    CompListValue * cType = atlantisGetPlantType (s);
    CompListValue * cNumber = atlantisGetPlantNumber (s);
    CompListValue * cSize = atlantisGetPlantSize (s);
    CompListValue * cColor = atlantisGetPlantColor (s);

    as->speedFactor = atlantisGetSpeedFactor (s);

    as->hsize = s->hsize * cs->nOutput;

    as->arcAngle = 360.0f / as->hsize;
    as->radius = (100000 - 1) * cs->distance /
		 cosf (0.5 * (as->arcAngle * toRadians));
    as->topDistance = (100000 - 1) * cs->distance;
    as->ratio = (atlantisGetRescaleWidth (s) ? ((float) s->width) /
		((float) s->height) : 1);
    as->sideDistance = as->topDistance * as->ratio;
    /* the 100000 comes from scaling by 0.00001 ( = (1/0.00001) ) */

    num = MIN (cType->nValue, cNumber->nValue);
    num = MIN (num, cSize->nValue);
    num = MIN (num, cColor->nValue);

    for (k = 0; k < num; k++)
    {
	for (j = 0; j < cNumber->value[k].i; j++)
	{
	    int size = cSize->value[k].i;

	    switch (cType->value[k].i) {
	    case 0:
	    case 1:
		coral = &(as->coral[i]);

		coral->size = (randf (sqrtf(size)) + size);
		coral->type = cType->value[k].i;

		if (j == 0)
		    setSimilarColor4us (coral->color, cColor->value[k].c,
		                        0.2, 0.2);
		else
		    setSimilarColor (coral->color, as->coral[i - j].color,
		                     0.2, 0.2);

		coral->psi = randf (360);

		setRandomLocation (s, &(coral->x), &(coral->y), 3 * size);
		coral->z = -50000;
		i++;
		break;

	    case 2:
		aerator = &(as->aerator[i2]);

		aerator->size = randf (sqrtf (size)) + size;
		aerator->type = cType->value[k].i;

		if (j == 0)
		    setSimilarColor4us (aerator->color, cColor->value[k].c,
		                        0, 0);
		else
		    setSimilarColor (aerator->color, as->aerator[i2-j].color,
		                     0.0, 0.0);

		setRandomLocation (s, &(aerator->x), &(aerator->y), size);
		aerator->z = -50000;

		for (bi = 0; bi < aerator->numBubbles; bi++)
		{
		    aerator->bubbles[bi].size = size;
		    aerator->bubbles[bi].x = aerator->x;
		    aerator->bubbles[bi].y = aerator->y;
		    aerator->bubbles[bi].z = aerator->z;
		    aerator->bubbles[bi].speed = 100 + randf (150);
		    aerator->bubbles[bi].offset = randf (2 * PI);
		    aerator->bubbles[bi].counter = 0;
		}

		i2++;
		break;
	    }
	}
    }

}

static void
freeAtlantis (CompScreen *s)
{
    ATLANTIS_SCREEN (s);

    int i;

    if (as->fish)
	free (as->fish);
    if (as->crab)
	free (as->crab);
    if (as->coral)
	free (as->coral);

    if (as->aerator)
    {
	for (i = 0; i < as->numAerators; i++)
	{
	    if (as->aerator[i].bubbles)
		free (as->aerator[i].bubbles);
	}

	free (as->aerator);
    }

    freeWater (as->water);
    freeWater (as->ground);

    as->fish = NULL;
    as->crab = NULL;
    as->coral= NULL;
    as->aerator = NULL;

    freeModels(s);
}

static void
updateAtlantis (CompScreen *s)
{
    freeAtlantis (s);
    initAtlantis (s);
}
static void
atlantisScreenOptionChange (CompScreen *s,
                            CompOption *opt,
                            AtlantisScreenOptions num)
{
    updateAtlantis (s);
}
static void
atlantisSpeedFactorOptionChange (CompScreen *s,
                                 CompOption *opt,
                                 AtlantisScreenOptions num)
{
    ATLANTIS_SCREEN (s);
    as->speedFactor = atlantisGetSpeedFactor (s);
}

static void
atlantisLightingOptionChange (CompScreen *s,
                              CompOption *opt,
                              AtlantisScreenOptions num)
{
    atlantisInitLightPosition (s);
}

static void
atlantisLowPolyOptionChange (CompScreen *s,
                             CompOption *opt,
                             AtlantisScreenOptions num)
{
    freeModels (s);
    loadModels (s);
}

static void
atlantisClearTargetOutput (CompScreen *s,
                           float xRotate,
                           float vRotate)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (as, cs, clearTargetOutput);
    (*cs->clearTargetOutput)(s, xRotate, vRotate);
    WRAP (as, cs, clearTargetOutput, atlantisClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static void
setLightPosition (CompScreen *s,
                  GLenum light)
{
    float position[] = { 0.0, 0.0, 1.0, 0.0 };
    float angle = atlantisGetLightInclination(s) * toRadians;

    if (atlantisGetRotateLighting (s))
	angle = 0;

    position[1] = sinf (angle);
    position[2] = cosf (angle);

    glLightfv (light, GL_POSITION, position);
}

static void
atlantisInitLightPosition(CompScreen *s)
{
    glPushMatrix ();
    glLoadIdentity ();
    setLightPosition (s, GL_LIGHT1);
    glPopMatrix();
}

static void
atlantisPaintInside(CompScreen *s,
		    const ScreenPaintAttrib *sAttrib,
		    const CompTransform *transform,
		    CompOutput *output,
		    int size)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int i, j;

    float scale;

    static const float mat_shininess[] = { 60.0 };
    static const float mat_specular[] = { 0.6, 0.6, 0.6, 1.0 };
    static const float mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float mat_ambient[] = { 0.8, 0.8, 0.9, 1.0 };

    static const float lmodel_localviewer[] = { 0.0 };
    static const float lmodel_twoside[] = { 0.0 };
    static       float lmodel_ambient[] = { 0.4, 0.4, 0.4, 0.4 };

    ScreenPaintAttrib sA = *sAttrib;
    CompTransform mT = *transform;

    int new_hsize = s->hsize * cs->nOutput;

    int drawDeformation = (as->oldProgress == 0.0f ? getCurrentDeformation(s) :
						     getDeformationMode (s));

    if (atlantisGetShowWater(s))
	as->waterHeight = atlantisGetWaterHeight(s) * 100000 - 50000;
    else
	as->waterHeight = 50000;

    if (new_hsize < as->hsize)
	updateAtlantis (s);
    else if (new_hsize > as->hsize)
    { /* let fish swim in their expanded enclosure without fully resetting */
	initWorldVariables (s);
    }

    if (atlantisGetShowWater (s) || atlantisGetShowWaterWire (s) ||
	atlantisGetShowGround (s))
    {
	updateDeformation (s, drawDeformation);
	updateHeight (as->water, atlantisGetShowGround (s) ? as->ground : NULL,
	              atlantisGetWaveRipple(s), drawDeformation);
    }

    sA.yRotate += cs->invert * (360.0f / size) * (cs->xRotations -
	          (s->x * cs->nOutput));

    (*s->applyScreenTransform)(s, &sA, output, &mT);

    glPushMatrix();

    glLoadMatrixf (mT.m);

    if (!atlantisGetRotateLighting (s))
	setLightPosition(s, GL_LIGHT1);

    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);

    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    Bool enabledCull = FALSE;

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

    glEnable (GL_BLEND);
    glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    for (i=0; i<4; i++)
	lmodel_ambient[i] = atlantisGetLightAmbient(s);

    glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
    glLightModelfv (GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    if (glIsEnabled (GL_CULL_FACE))
    {
	enabledCull = TRUE;
    }

    if (atlantisGetShowWater (s))
    {
	int cull;

	glGetIntegerv (GL_CULL_FACE_MODE, &cull);
	glEnable (GL_CULL_FACE);

	glCullFace (~cull & (GL_FRONT | GL_BACK));
	setWaterMaterial (atlantisGetWaterColor (s));
	drawWater (as->water, TRUE, FALSE, drawDeformation);
	glCullFace (cull);
    }

    if (atlantisGetShowGround (s))
    {
	setGroundMaterial (atlantisGetGroundColor (s));

	if (atlantisGetRenderWaves (s) && atlantisGetShowWater (s) &&
	    !atlantisGetWaveRipple (s))
	    drawGround (as->water, as->ground, drawDeformation);
	else
	    drawGround (NULL, as->ground, drawDeformation);
    }

    glPushMatrix();

    glScalef (1.0f / as->ratio, 1.0f, 1.0f / as->ratio);

    glColor4usv (defaultColor);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    glEnable  (GL_NORMALIZE);
    glEnable  (GL_DEPTH_TEST);
    glEnable  (GL_COLOR_MATERIAL);
    glEnable  (GL_LIGHTING);
    glEnable  (GL_LIGHT1);
    glDisable (GL_LIGHT0);

    glShadeModel(GL_SMOOTH);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glScalef (0.00001f, 0.00001f, 0.00001f);

    for (i = 0; i < as->numCrabs; i++)
    {
	glPushMatrix ();

	CrabTransform (& (as->crab[i]));

	scale = as->crab[i].size;
	scale /= 6000.0f;
	glScalef (scale, scale, scale);
	glColor4fv (as->crab[i].color);

	initDrawCrab ();
	glCallList (as->crabDisplayList);
	finDrawCrab ();

	glPopMatrix ();
    }

    for (i = 0; i < as->numCorals; i++)
    {
	glPushMatrix ();

	glTranslatef (as->coral[i].y, as->coral[i].z, as->coral[i].x);
	glRotatef (-as->coral[i].psi, 0.0, 1.0, 0.0);

	scale = as->coral[i].size;
	scale /= 6000.0f;
	glScalef (scale, scale, scale);
	glColor4fv (as->coral[i].color);

	switch (as->coral[i].type) {
	case 0:
	    initDrawCoral ();
	    glCallList (as->coralDisplayList);
	    finDrawCoral ();
	    break;
	case 1:
	    initDrawCoral2 ();
	    glCallList (as->coral2DisplayList);
	    finDrawCoral2 ();
	    break;
	}

	glPopMatrix ();
    }

    for (i = 0; i < as->numFish; i++)
    {
	glPushMatrix ();
	FishTransform (& (as->fish[i]));
	scale = as->fish[i].size;
	scale /= 6000.0f;
	glScalef (scale, scale, scale);
	glColor4fv (as->fish[i].color);

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

	case BUTTERFLYFISH:
	    initDrawBFish (as->fish[i].color);
	    AnimateBFish  (as->fish[i].htail);
	    DrawAnimatedBFish ();
	    finDrawBFish ();
	    break;

	case CHROMIS:
	    initDrawChromis (as->fish[i].color);
	    AnimateChromis (as->fish[i].htail);
	    DrawAnimatedChromis ();
	    finDrawChromis ();
	    break;

	case CHROMIS2:
	    initDrawChromis2 (as->fish[i].color);
	    AnimateChromis   (as->fish[i].htail);
	    DrawAnimatedChromis ();
	    finDrawChromis ();
	    break;

	case CHROMIS3:
	    initDrawChromis3 (as->fish[i].color);
	    AnimateChromis   (as->fish[i].htail);
	    DrawAnimatedChromis ();
	    finDrawChromis ();
	    break;

	case FISH:
	    initDrawFish (as->fish[i].color);
	    AnimateFish  (as->fish[i].htail);
	    DrawAnimatedFish ();
	    finDrawFish ();
	    break;

	case FISH2:
	    initDrawFish2 (as->fish[i].color);
	    AnimateFish2  (as->fish[i].htail);
	    DrawAnimatedFish2 ();
	    finDrawFish2 ();
	    break;

	default:
	    break;
	}

	glPopMatrix();
    }

    glEnable(GL_CULL_FACE);

    for (i = 0; i < as->numAerators; i++)
    {
	for (j = 0; j < as->aerator[i].numBubbles; j++)
	{
	    glPushMatrix ();

	    BubbleTransform (&(as->aerator[i].bubbles[j]));
	    scale = as->aerator[i].bubbles[j].size;

	    glScalef (scale, scale, scale);
	    glColor4fv (as->aerator[i].color);

	    glCallList (as->bubbleDisplayList);

	    glPopMatrix ();
	}
    }

    glPopMatrix ();

    if (atlantisGetShowWater (s) || atlantisGetShowWaterWire (s))
    {
	glEnable (GL_CULL_FACE);
	setWaterMaterial (atlantisGetWaterColor (s));
	drawWater (as->water, atlantisGetShowWater (s),
		atlantisGetShowWaterWire (s), drawDeformation);
    }


    if (atlantisGetShowGround (s))
    {
	setGroundMaterial (atlantisGetGroundColor (s));
	drawBottomGround (as->ground, cs->distance, -0.5, drawDeformation);
    }
    else if (atlantisGetShowWater (s))
    {
	setWaterMaterial (atlantisGetWaterColor (s));
	drawBottomWater (as->water, cs->distance, -0.5, drawDeformation);
    }


    glDisable (GL_LIGHT1);
    glDisable (GL_NORMALIZE);

    if (!s->lighting)
	glDisable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

    glPopMatrix ();

    glPopAttrib ();

    as->damage = TRUE;

    UNWRAP (as, cs, paintInside);
    (*cs->paintInside)(s, sAttrib, transform, output, size);
    WRAP (as, cs, paintInside, atlantisPaintInside);
}

static void
atlantisPreparePaintScreen (CompScreen *s,
                            int ms)
{
    ATLANTIS_SCREEN (s);

    int i, j;

    Bool currentDeformation = getCurrentDeformation (s);
    int oldhsize = as->hsize;

    updateWater (s, (float) ms / 1000.0f);
    updateGround (s, (float) ms / 1000.0f);

    /* temporary change for animals inside */
    if (currentDeformation == DeformationCylinder && as->oldProgress > 0.9)
    {
	as->hsize *= 32 / as->hsize;
	as->arcAngle = 360.0f / as->hsize;
	as->sideDistance = as->radius * as->ratio;
    }
    else if (currentDeformation == DeformationSphere)
    {
	/* treat enclosure as a cylinder */
	as->hsize *= 32 / as->hsize;
	as->arcAngle = 360.0f / as->hsize;
	as->sideDistance = as->radius * as->ratio;

    }

    for (i = 0; i < as->numFish; i++)
    {
	FishPilot (s, i);

	/* animate fish tails */
	if (as->fish[i].type <= FISH2)
	{
	    as->fish[i].htail = fmodf (as->fish[i].htail + 0.00025 *
	                               as->fish[i].speed * as->speedFactor, 1);
	}
    }

    for (i = 0; i < as->numCrabs; i++)
    {
	CrabPilot (s, i);
    }

    for (i = 0; i < as->numCorals; i++)
    {
	as->coral[i].z = getGroundHeight (s, as->coral[i].x, as->coral[i].y);
    }

    for (i = 0; i < as->numAerators; i++)
    {
	aeratorRec * aerator = &(as->aerator[i]);
	float bottom = getGroundHeight (s, aerator->x, aerator->y);

	if (aerator->z < bottom)
	{
	    for (j = 0; j < aerator->numBubbles; j++)
	    {
		if (aerator->bubbles[j].counter == 0)
		    aerator->bubbles[j].z = bottom;
	    }
	}
	aerator->z = bottom;
	for (j = 0; j < aerator->numBubbles; j++)
	{
	    BubblePilot(s, i, j);
	}
    }

    as->hsize = oldhsize;
    as->arcAngle = 360.0f / as->hsize;
    as->sideDistance = as->topDistance * as->ratio;

    UNWRAP (as, s, preparePaintScreen);
    (*s->preparePaintScreen)(s, ms);
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
    (*s->donePaintScreen)(s);
    WRAP (as, s, donePaintScreen, atlantisDonePaintScreen);
}

static Bool
atlantisInitDisplay (CompPlugin *p,
                     CompDisplay *d)
{
    AtlantisDisplay *ad;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||!checkPluginABI ("cube",
	    CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    ad = malloc (sizeof(AtlantisDisplay));

    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    d->base.privates[atlantisDisplayPrivateIndex].ptr = ad;

    return TRUE;
}

static void
atlantisFiniDisplay (CompPlugin *p,
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
    static const float ambient[]  = { 0.0, 0.0, 0.0, 0.0 };
    static const float diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
    static const float specular[] = { 0.6, 0.6, 0.6, 1.0 };

    AtlantisScreen *as;

    ATLANTIS_DISPLAY (s->display);
    CUBE_SCREEN (s);

    as = malloc (sizeof (AtlantisScreen));

    if (!as)
	return FALSE;

    s->base.privates[ad->screenPrivateIndex].ptr = as;

    as->damage = FALSE;

    glLightfv (GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT1, GL_SPECULAR, specular);
    atlantisInitLightPosition (s);

    initAtlantis (s);

    atlantisSetSpeedFactorNotify  (s, atlantisSpeedFactorOptionChange);

    atlantisSetLowPolyNotify (s, atlantisLowPolyOptionChange);

    atlantisSetCreatureNumberNotify (s, atlantisScreenOptionChange);
    atlantisSetCreatureSizeNotify   (s, atlantisScreenOptionChange);
    atlantisSetCreatureColorNotify  (s, atlantisScreenOptionChange);
    atlantisSetCreatureTypeNotify   (s, atlantisScreenOptionChange);

    atlantisSetPlantNumberNotify (s, atlantisScreenOptionChange);
    atlantisSetPlantSizeNotify   (s, atlantisScreenOptionChange);
    atlantisSetPlantColorNotify  (s, atlantisScreenOptionChange);
    atlantisSetPlantTypeNotify   (s, atlantisScreenOptionChange);

    atlantisSetRescaleWidthNotify (s, atlantisScreenOptionChange);

    atlantisSetRotateLightingNotify   (s, atlantisLightingOptionChange);
    atlantisSetLightInclinationNotify (s, atlantisLightingOptionChange);


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
    atlantisDisplayPrivateIndex = allocateDisplayPrivateIndex ();

    if (atlantisDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
atlantisFini (CompPlugin * p)
{
    if (atlantisDisplayPrivateIndex >= 0)
	freeDisplayPrivateIndex (atlantisDisplayPrivateIndex);
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
