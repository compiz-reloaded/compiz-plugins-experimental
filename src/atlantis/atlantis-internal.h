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

#define RAD 57.295
#define RRAD 0.01745

#define LRAND()                 ((long) (random() & 0x7fffffff))
#define NRAND(n)                ((int) (LRAND() % (n)))
#define MAXRAND                 (2147483648.0) /* unsigned 1<<31 as a float */


/* default values */
#define NUM_SHARKS 4
#define SHARKSPEED 100
#define SHARKSIZE 6000

#include <math.h>
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

#define SHARK     0
#define WHALE     1
#define DOLPHIN   2
#define FISH      3

typedef struct _fishRec
{
    float       x, y, z, phi, theta, psi, v;
    float       xt, yt, zt;
    float       htail, vtail;
    float       dtheta;
    int         spurt, attack;
    int         sign;
    int         size;
    float       speed;
    int         type;
    float       color[3];
}
fishRec;

typedef struct _Vertex
{
    float v[3];
    float n[3];
}
Vertex;

typedef struct _Water
{
    int      size;
    float    distance;
    int      sDiv;

    float  bh;
    float  wa;
    float  swa;
    float  wf;
    float  swf;

    Vertex       *vertices;
    unsigned int *indices;

    unsigned int nVertices;
    unsigned int nIndices;

    unsigned int nSVer;
    unsigned int nSIdx;
    unsigned int nWVer;
    unsigned int nWIdx;

    float    wave1;
    float    wave2;
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
    CubePaintInsideProc       paintInside;

    Bool damage;

    int         numFish;
    fishRec    *fish;

    Water *water;
    Water *ground;
}
AtlantisScreen;

void FishTransform (fishRec *);
void FishPilot (fishRec *, float);
void FishMiss (AtlantisScreen *, int);
void DrawWhale (fishRec *, int);
void DrawShark (fishRec *, int);
void DrawDolphin (fishRec *, int);

void
updateWater (CompScreen *s, float time);

void
updateGround (CompScreen *s, float time);

void
updateHeight (Water *w);

void
freeWater (Water *w);

void
drawWater (Water *w, Bool full, Bool wire);

void
drawGround (Water *w, Water *g);

void
drawBottomGround (int size, float distance, float bottom);

Bool
isInside (CompScreen *s, float x, float y, float z);

float
getHeight (Water *w, float x, float z);

#endif
