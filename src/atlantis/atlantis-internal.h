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

#ifndef _ATLANTIS_INTERNAL_H
#define _ATLANTIS_INTERNAL_H

#define LRAND()                 ((long) (random() & 0x7fffffff))
#define NRAND(n)                ((int) (LRAND() % (n)))
#define MAXRAND                 (2147483648.0) /* unsigned 1<<31 as a float */

#include <math.h>
#include <float.h>

/* some constants */
#define PI     M_PI
#define PIdiv2 M_PI_2
#define toDegrees (180.0f * M_1_PI)
#define toRadians (M_PI / 180.0f)

/* for original atlantis screensaver models */
#define RAD toDegrees
#define RRAD toRadians

/* return random number in range [0,x) */
#define randf(x) ((float) (rand()/(((double)RAND_MAX + 1)/(x))))


#include <compiz-core.h>
#include <compiz-cube.h>

extern int atlantisDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;

#define GET_ATLANTIS_DISPLAY(d) \
    ((AtlantisDisplay *) (d)->base.privates[atlantisDisplayPrivateIndex].ptr)
#define ATLANTIS_DISPLAY(d) \
    AtlantisDisplay *ad = GET_ATLANTIS_DISPLAY(d);

#define GET_ATLANTIS_SCREEN(s, ad) \
    ((AtlantisScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)
#define ATLANTIS_SCREEN(s) \
    AtlantisScreen *as = GET_ATLANTIS_SCREEN(s, GET_ATLANTIS_DISPLAY(s->display))


/* matching value from atlantis.xml.in */
#define BUTTERFLYFISH	0
#define CHROMIS		1
#define CHROMIS2	2
#define CHROMIS3	3
#define FISH		4
#define FISH2		5
#define DOLPHIN		6
#define SHARK		7
#define WHALE		8
#define CRAB		9


/*
 * groups to separate fish for calculating boidsAngles
 * at each time slice only one of the groups will have the boidsAngles updated
*/
#define NUM_GROUPS 6


/* matching values from cubeaddon plugin */
#define DeformationNone		0
#define DeformationCylinder	1
#define DeformationSphere	2


typedef struct _fishRec
{
    float x, y, z, theta, psi, v;
    float htail, vtail;
    float dtheta;
    int spurt, attack;
    int size;
    float speed;
    int type;
    float color[4];
    int group;
    int boidsCounter; /* so that boidsAngles aren't computed every time step */
    float boidsPsi;
    float boidsTheta;
    int smoothTurnCounter;
    float smoothTurnAmount; /* psi direction */
    float smoothTurnTh;     /* theta direction */
    float prevRandPsi;
    float prevRandTh;
}
fishRec;

typedef struct _crabRec
{
    float x, y, z, theta, psi;
    int size;
    float speed;
    float color[4];
    int scuttleAmount;
    float scuttlePsi;
    float scuttleTheta;
    Bool isFalling;
}
crabRec;

typedef struct _coralRec
{
    float x, y, z, psi;
    int size;
    int type;
    float color[4];
}
coralRec;

typedef struct _Bubble
{
    float x, y, z;
    float size;
    float speed;
    float counter;
    float offset;
}
Bubble;

typedef struct _aeratorRec
{
    float x, y, z;
    int size;
    int type;
    float color[4];
    Bubble *bubbles;

    int numBubbles;
}
aeratorRec;

typedef struct _Vertex
{
    float v[3];
    float n[3];
}
Vertex;

typedef struct _Water
{
    int size;
    float distance;
    int sDiv;

    float bh;
    float wa;
    float swa;
    float wf;
    float swf;

    Vertex *vertices;
    unsigned int *indices;

    Vertex *vertices2; /* for extra side wall detail in sphere deformation */
    unsigned int *indices2;

    int *rippleFactor;
    int rippleTimer;

    unsigned int nVertices;
    unsigned int nIndices;

    unsigned int nSVer;
    unsigned int nSIdx;
    unsigned int nWVer;
    unsigned int nWIdx;
    unsigned int nBIdx;

    unsigned int nWVer2;
    unsigned int nWIdx2;
    unsigned int nBIdx2;

    float wave1;
    float wave2;
}
Water;

typedef struct _AtlantisDisplay
{
    int screenPrivateIndex;
}
AtlantisDisplay;

typedef struct _AtlantisScreen
{
    DonePaintScreenProc donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc paintInside;

    Bool damage;

    int numFish;
    int numCrabs;
    int numCorals;
    int numAerators;

    fishRec *fish;
    crabRec *crab;
    coralRec *coral;
    aeratorRec *aerator;

    Water *water;
    Water *ground;

    float waterHeight;  /* water surface height */

    int hsize;
    float sideDistance; /* perpendicular distance to side wall from center */
    float topDistance;  /* perpendicular distance to top wall from center */
    float radius;       /* radius on which the hSize points lie */
    float arcAngle;   	/* 360 degrees / horizontal size */
    float ratio;        /* screen width to height */

    float speedFactor;  /* multiply fish/crab speeds by this value */

    float oldProgress;

    GLuint crabDisplayList;
    GLuint coralDisplayList;
    GLuint coral2DisplayList;
    GLuint bubbleDisplayList;
}
AtlantisScreen;

void
updateWater(CompScreen *s, float time);

void
updateGround(CompScreen *s, float time);

void
updateDeformation (CompScreen *s, int currentDeformation);

void
updateHeight(Water *w, Water *w2, Bool, int currentDeformation);

void
freeWater(Water *w);

void
drawWater(Water *w, Bool full, Bool wire, int currentDeformation);

void
drawGround(Water *w, Water *g, int currentDeformation);

void
drawBottomGround(Water *w, float distance, float bottom, int currentDeformation);

void
drawBottomWater(Water *w, float distance, float bottom, int currentDeformation);

void
setWaterMaterial (unsigned short *);

void
setGroundMaterial (unsigned short *);

Bool
isInside(CompScreen *s, float x, float y, float z);

float
getHeight(Water *w, float x, float z);

float
getGroundHeight (CompScreen *s, float x, float z);

void
FishTransform(fishRec *);

void
FishPilot(CompScreen *, int);

void
CrabTransform(crabRec *);

void
CrabPilot(CompScreen *, int);

void
BubbleTransform(Bubble *);

void
BubblePilot(CompScreen *, int, int);

void
BoidsAngle(CompScreen *, int);

void
RenderWater(int, float, Bool, Bool);

void
DrawWhale(fishRec *, int);

void
DrawShark(fishRec *, int);

void
DrawDolphin(fishRec *, int);

void
AnimateBFish(float);

void
DrawAnimatedBFish(void);

void
initDrawBFish(float *);

void
finDrawBFish(void);

void
AnimateChromis(float);

void
DrawAnimatedChromis(void);

void
initDrawChromis(float *);

void
initDrawChromis2(float *);

void
initDrawChromis3(float *);

void
finDrawChromis(void);

void
DrawAnimatedFish(void);

void
AnimateFish(float);

void
initDrawFish(float *);

void
finDrawFish(void);

void
DrawAnimatedFish2(void);

void
AnimateFish2(float);

void
initDrawFish2(float *);

void
finDrawFish2(void);

void
DrawCrab(int);

void
initDrawCrab(void);

void
finDrawCrab(void);

void
DrawCoral(int);

void
DrawCoralLow(int);

void
initDrawCoral(void);

void
finDrawCoral(void);

void
DrawCoral2(int);

void
DrawCoral2Low(int);

void
initDrawCoral2(void);

void
finDrawCoral2(void);

void
DrawBubble(int, int);


/* utility methods */
int
getCurrentDeformation(CompScreen *s);

int
getDeformationMode(CompScreen *s);

float
symmDistr(void); /* symmetric distribution */

void
setColor(float *, float, float, float, float, float, float);

void
setSimilarColor(float *, float*, float, float);

void
setSimilarColor4us(float *, unsigned short *, float, float);

void
setSpecifiedColor(float *, int);

void
setRandomLocation(CompScreen *, float *, float *, float);

void
setMaterialAmbientDiffuse (float *, float, float);

void
setMaterialAmbientDiffuse4us (unsigned short *, float, float);

void
copyColor (float *, float *, float);

void
convert4usTof (unsigned short *, float *);


#endif
