/*
 *
 * Compiz 3d plugin
 *
 * 3d.c
 *
 * Copyright : (C) 2006 by Roi Cohen
 * E-mail    : roico12@gmail.com
 *
 * Modified by : Dennis Kasprzyk <onestone@opencompositing.org>
 *               Danny Baumann <maniac@opencompositing.org>
 *               Robert Carr <racarr@beryl-project.org>
 *               Diogo Ferreira <diogo@underdev.org>
 *		 Kevin Lange <klange@ogunderground.com>
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

/** TODO:
  1. Add 3d shadows / projections.
  2. Add an option to select z-order of windows not only by viewports,
     but also by screens.
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-cube.h>
#include "3d_options.h"

#define PI 3.14159265359f

static int displayPrivateIndex;
static int cubeDisplayPrivateIndex = -1;

typedef struct _tdDisplay
{
    int screenPrivateIndex;
} tdDisplay;

typedef struct _tdWindow
{
    Bool ftb;
    Bool is3D;

    float depth;

    CompWindow *next;
    CompWindow *prev;
} tdWindow;

typedef struct _tdScreen
{
    int windowPrivateIndex;

    PreparePaintScreenProc    preparePaintScreen;
    PaintOutputProc	      paintOutput;
    CubePostPaintViewportProc postPaintViewport;
    DonePaintScreenProc	      donePaintScreen;
    InitWindowWalkerProc      initWindowWalker;
    ApplyScreenTransformProc  applyScreenTransform;
    PaintWindowProc           paintWindow;

    CompWindow *first;
    CompWindow *last;

    Bool  active;
    Bool  painting3D;
    float currentScale;

    float basicScale;
    float maxDepth;

    CompTransform bTransform;
} tdScreen;

#define GET_TD_DISPLAY(d)       \
    ((tdDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define TD_DISPLAY(d)   \
    tdDisplay *tdd = GET_TD_DISPLAY (d)

#define GET_TD_SCREEN(s, tdd)   \
    ((tdScreen *) (s)->base.privates[(tdd)->screenPrivateIndex].ptr)

#define TD_SCREEN(s)    \
    tdScreen *tds = GET_TD_SCREEN (s, GET_TD_DISPLAY (s->display))

#define GET_TD_WINDOW(w, tds)                                     \
    ((tdWindow *) (w)->base.privates[(tds)->windowPrivateIndex].ptr)

#define TD_WINDOW(w)    \
    tdWindow *tdw = GET_TD_WINDOW  (w,                     \
		    GET_TD_SCREEN  (w->screen,             \
		    GET_TD_DISPLAY (w->screen->display)))

static Bool
windowIs3D (CompWindow *w)
{
    if (w->attrib.override_redirect)
	return FALSE;

    if (!(w->shaded || w->attrib.map_state == IsViewable))
	return FALSE;

    if (w->state & (CompWindowStateSkipPagerMask |
		    CompWindowStateSkipTaskbarMask))
	return FALSE;
	
    if (!matchEval (tdGetWindowMatch (w->screen), w))
	return FALSE;

    return TRUE;
}

static void
tdPreparePaintScreen (CompScreen *s,
		      int        msSinceLastPaint)
{
    CompWindow *w;
    float      amount;
    Bool       active;

    TD_SCREEN (s);
    CUBE_SCREEN (s);

    active = (cs->rotationState != RotationNone) &&
	     !(tdGetManualOnly(s) && (cs->rotationState != RotationManual));

    amount = ((float)msSinceLastPaint * tdGetSpeed (s) / 1000.0);
    if (active)
    {
	float maxDiv = (float) tdGetMaxWindowSpace (s) / 100;
	float minScale = (float) tdGetMinCubeSize (s) / 100;

	tds->maxDepth = 0;
	for (w = s->windows; w; w = w->next)
	{
	    TD_WINDOW (w);
	    tdw->is3D = FALSE;
	    tdw->depth = 0;

	    if (!windowIs3D (w))
		continue;

	    tdw->is3D = TRUE;
	    tds->maxDepth++;
	    tdw->depth = tds->maxDepth;
	}

	minScale =  MAX (minScale, 1.0 - (tds->maxDepth * maxDiv));
	if (cs->invert == 1)
	    tds->basicScale = MAX (minScale, tds->basicScale - amount);
	else
	    tds->basicScale = MIN (2 - minScale, tds->basicScale + amount);
    }
    else
    {
	if (cs->invert == 1)
	    tds->basicScale = MIN (1.0, tds->basicScale + amount);
	else
	    tds->basicScale = MAX (1.0, tds->basicScale - amount);
    }

    /* comparing float values with != is error prone, so better cache
       the comparison and allow a small difference */
    tds->active       = (fabs (tds->basicScale - 1.0f) > 1e-4);
    tds->currentScale = tds->basicScale;

    UNWRAP (tds, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (tds, s, preparePaintScreen, tdPreparePaintScreen);

    cs->paintAllViewports |= tds->active;
}

/* forward declaration */
static Bool tdPaintWindow (CompWindow              *w,
			   const WindowPaintAttrib *attrib,
			   const CompTransform     *transform,
			   Region                  region,
			   unsigned int            mask);

#define DOBEVEL(corner) (tdGetBevel##corner (s) ? bevel : 0)

#define ADDQUAD(x1,y1,x2,y2)                                      \
	point.x = x1; point.y = y1;                               \
	matrixMultiplyVector (&tPoint, &point, transform);        \
	glVertex4fv (tPoint.v);                                   \
	point.x = x2; point.y = y2;                               \
	matrixMultiplyVector (&tPoint, &point, transform);        \
	glVertex4fv (tPoint.v);                                   \
	matrixMultiplyVector (&tPoint, &point, &tds->bTransform); \
	glVertex4fv (tPoint.v);                                   \
	point.x = x1; point.y = y1;                               \
	matrixMultiplyVector (&tPoint, &point, &tds->bTransform); \
	glVertex4fv (tPoint.v);                                   \

#define ADDBEVELQUAD(x1,y1,x2,y2,m1,m2)             \
	point.x = x1; point.y = y1;                 \
	matrixMultiplyVector (&tPoint, &point, m1); \
	glVertex4fv (tPoint.v);                     \
	matrixMultiplyVector (&tPoint, &point, m2); \
	glVertex4fv (tPoint.v);                     \
	point.x = x2; point.y = y2;                 \
	matrixMultiplyVector (&tPoint, &point, m2); \
	glVertex4fv (tPoint.v);                     \
	matrixMultiplyVector (&tPoint, &point, m1); \
	glVertex4fv (tPoint.v);                     \

static Bool
tdPaintWindowWithDepth (CompWindow              *w,
		     	const WindowPaintAttrib *attrib,
			const CompTransform     *transform,
			Region                  region,
			unsigned int            mask)
{
    Bool       wasCulled;
    Bool       status;
    int        wx, wy, ww, wh;
    int        bevel, cull;
    CompScreen *s = w->screen;
    CompVector point, tPoint;

    TD_SCREEN (s);

    wasCulled = glIsEnabled (GL_CULL_FACE);

    wx = w->attrib.x - w->input.left;
    wy = w->attrib.y - w->input.top;

    ww = w->width + w->input.left + w->input.right;
    wh = w->height + w->input.top + w->input.bottom;

    bevel = tdGetBevel (s);

    glGetIntegerv (GL_CULL_FACE_MODE, &cull);

    if (ww && wh && !(mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK))
    {
	TD_WINDOW (w);

	if (!tdw->ftb)
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_FRONT);

	    UNWRAP (tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, &tds->bTransform, region,
					mask | PAINT_WINDOW_TRANSFORMED_MASK);
	    WRAP (tds, s, paintWindow, tdPaintWindow);
	}
	else
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_BACK);

	    UNWRAP (tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, transform, region, mask);
	    WRAP (tds, s, paintWindow, tdPaintWindow);
	}

	/* Paint window depth. */
	glPushMatrix ();
	glLoadIdentity ();
	
	glDisable (GL_CULL_FACE);
	glEnable (GL_BLEND);
    	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin (GL_QUADS);
	unsigned short *c;
	if (w->id == s->display->activeWindow)
	    c = tdGetWidthColor(s);
	else
	    c = tdGetWidthColorInactive(s);
	int temp = c[3] * w->paint.opacity;
	temp /= 0xffff;
	glColor4us(c[0],c[1],c[2],temp);

	point.z = 0.0f;
	point.w = 1.0f;

	/* Top */
	ADDQUAD (wx + DOBEVEL (Topleft), wy + 0.01,
		 wx + ww - DOBEVEL (Topright), wy + 0.01);

	/* Bottom */
	ADDQUAD (wx + DOBEVEL (Bottomleft), wy + wh - 0.01,
		 wx + ww - DOBEVEL (Bottomright), wy + wh - 0.01);

	/* Left */
	ADDQUAD (wx + 0.01, wy + DOBEVEL (Topleft),
		 wx + 0.01, wy + wh - DOBEVEL (Bottomleft));

	/* Right */
	ADDQUAD (wx + ww - 0.01, wy + DOBEVEL (Topright),
		 wx + ww - 0.01, wy + wh - DOBEVEL (Bottomright));\
		 
	/* Top left bevel */
	if (tdGetBevelTopleft (s))
	{
	    ADDBEVELQUAD (wx, wy + bevel,
			  wx + bevel / 2.0f, wy + bevel - bevel / 1.2f,
			  &tds->bTransform, transform);

	    ADDBEVELQUAD (wx + bevel / 2.0f, wy + bevel - bevel / 1.2f,
			  wx + bevel, wy,
			  transform, &tds->bTransform);

	}

	/* Bottom left bevel */
	if (tdGetBevelBottomleft (s))
	{
	    ADDBEVELQUAD (wx, wy + wh - bevel,
			  wx + bevel / 2.0f, wy + wh - bevel + bevel / 1.2f,
			  transform, &tds->bTransform);

	    ADDBEVELQUAD (wx + bevel / 2.0f, wy + wh - bevel + bevel / 1.2f,
			  wx + bevel, wy + wh,
			  &tds->bTransform, transform);
	}

	/* Bottom right bevel */
	if (tdGetBevelBottomright (s))
	{
	    ADDBEVELQUAD (wx + ww - bevel, wy + wh,
			  wx + ww - bevel / 2.0f,
			  wy + wh - bevel + bevel / 1.2f,
			  transform, &tds->bTransform);

	    ADDBEVELQUAD (wx + ww - bevel / 2.0f,
			  wy + wh - bevel + bevel / 1.2f,
			  wx + ww, wy + wh - bevel,
			  &tds->bTransform, transform);

	}

	/* Top right bevel */
	if (tdGetBevelTopright (s))
	{
	    ADDBEVELQUAD (wx + ww - bevel, wy,
			  wx + ww - bevel / 2.0f, wy + bevel - bevel / 1.2f,
			  transform, &tds->bTransform);

	    ADDBEVELQUAD (wx + ww - bevel / 2.0f, wy + bevel - bevel / 1.2f,
			  wx + ww, wy + bevel,
			  &tds->bTransform, transform);
	}
	glColor4usv (defaultColor);
    	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnd ();
	glPopMatrix ();

	if (tdw->ftb)
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_FRONT);

	    glPushMatrix ();
	    glLoadMatrixf (tds->bTransform.m);

	    UNWRAP (tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, &tds->bTransform, region,
					mask | PAINT_WINDOW_TRANSFORMED_MASK);
	    WRAP(tds, s, paintWindow, tdPaintWindow);

	    glPopMatrix ();
	}
	else
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_BACK);

	    UNWRAP(tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, transform, region, mask);
	    WRAP (tds, s, paintWindow, tdPaintWindow);
	}
    }
    else
    {
	UNWRAP(tds, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (tds, s, paintWindow, tdPaintWindow);
    }
    glCullFace (cull);

    if (!wasCulled)
	glDisable (GL_CULL_FACE);

    return status;
}

static Bool
tdPaintWindow (CompWindow              *w,
	       const WindowPaintAttrib *attrib,
	       const CompTransform     *transform,
	       Region                  region,
	       unsigned int            mask)
{
    Bool       status;
    CompScreen *s = w->screen;

    TD_SCREEN (s);
    TD_WINDOW (w);

    if (tdw->depth != 0.0f && !tds->painting3D && tds->active)
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

    if (tds->painting3D)
    {
	if (tdGetWidth (s) && (tdw->depth != 0.0f))
	{
	    status = tdPaintWindowWithDepth (w, attrib, transform,
					     region, mask);
	}
	else
	{
	    Bool wasCulled;

	    wasCulled = glIsEnabled (GL_CULL_FACE);
	    glDisable (GL_CULL_FACE);

	    UNWRAP (tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, transform, region, mask);
	    WRAP (tds, s, paintWindow, tdPaintWindow);

	    if (wasCulled)
		glEnable (GL_CULL_FACE);
	}
    }
    else
    {
	UNWRAP (tds, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP(tds, s, paintWindow, tdPaintWindow);
    }

    return status;
}

static void
tdApplyScreenTransform (CompScreen		*s,
			const ScreenPaintAttrib *sAttrib,
			CompOutput		*output,
			CompTransform	        *transform)
{
    TD_SCREEN (s);

    UNWRAP (tds, s, applyScreenTransform);
    (*s->applyScreenTransform) (s, sAttrib, output, transform);
    WRAP (tds, s, applyScreenTransform, tdApplyScreenTransform);

    matrixScale (transform,
		 tds->currentScale, tds->currentScale, tds->currentScale);
}

static void
tdAddWindow (CompWindow *w)
{
    TD_SCREEN (w->screen);
    TD_WINDOW (w);

    if (!tds->first)
    {
	tds->first = tds->last = w;
	return;
    }

    GET_TD_WINDOW (tds->last, tds)->next = w;
    tdw->prev = tds->last;
    tds->last = w;
}

static void
tdPostPaintViewport (CompScreen              *s,
		     const ScreenPaintAttrib *sAttrib,
		     const CompTransform     *transform,
		     CompOutput              *output,
		     Region                  region)
{
    TD_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (tds, cs, postPaintViewport);
    (*cs->postPaintViewport) (s, sAttrib, transform, output, region);
    WRAP (tds, cs, postPaintViewport, tdPostPaintViewport);

    if (tds->active)
    {
	CompTransform sTransform = *transform;
	CompWindow    *firstFTB = NULL;
	CompWindow    *w;
	CompWalker    walk;
	float         wDepth;
	float         pointZ = cs->invert * cs->distance;

	CompVector vPoints[3] = { { .v = { -0.5, 0.0, pointZ, 1.0 } },
	                          { .v = {  0.0, 0.5, pointZ, 1.0 } },
				  { .v = {  0.0, 0.0, pointZ, 1.0 } } };

	wDepth = -MIN((tdGetWidth (s)) / 30, (1.0 - tds->basicScale) /
					       tds->maxDepth);

	/* all non 3d windows first */
	tds->first = NULL;
	tds->last = NULL;

	for (w = s->windows; w; w = w->next)
	{
	    TD_WINDOW (w);

	    tdw->next = NULL;
	    tdw->prev = NULL;
	}

	for (w = s->windows; w; w = w->next)
	{
	    TD_WINDOW (w);

	    if (!tdw->is3D)
		tdAddWindow (w);
	}

	/* all BTF windows in normal order */
	for (w = s->windows; w && !firstFTB; w = w->next)
	{
	    TD_WINDOW (w);

	    if (!tdw->is3D)
		continue;

	    tds->currentScale = tds->basicScale +
		                (tdw->depth * ((1.0 - tds->basicScale) /
					       tds->maxDepth));

	    tdw->ftb = (*cs->checkOrientation) (s, sAttrib, transform,
						output, vPoints);

	    if (tdw->ftb)
		firstFTB = w;
	    else
		tdAddWindow (w);
	}

	/* all FTB windows in reversed order */
	if (firstFTB)
	{
	    for (w = s->reverseWindows; w && w != firstFTB->prev; w = w->prev)
	    {
		TD_WINDOW (w);

		if (!tdw->is3D)
		    continue;

		tdw->ftb = TRUE;

		tdAddWindow (w);
	    }
	}

	tds->currentScale = tds->basicScale;
	tds->painting3D   = TRUE;

	screenLighting (s, TRUE);

	(*s->initWindowWalker) (s, &walk);

	/* paint all windows from bottom to top */
	for (w = (*walk.first) (s); w; w = (*walk.next) (w))
	{
	    CompTransform mTransform = sTransform;
	    int           offX, offY;
	    unsigned int  newMask = PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK;

	    TD_WINDOW (w);

	    if (w->destroyed)
		continue;

	    if (!w->shaded)
	    {
		if (w->attrib.map_state != IsViewable || !w->damaged)
		    continue;
	    }

	    if (tdw->depth != 0.0f)
	    {
		tds->currentScale = tds->basicScale +
		                    (tdw->depth * ((1.0 - tds->basicScale) /
						   tds->maxDepth));

		tds->currentScale += wDepth;
		tds->bTransform   = sTransform;
		(*s->applyScreenTransform) (s, sAttrib, output,
					    &tds->bTransform);
		tds->currentScale -= wDepth;

		transformToScreenSpace (s, output, -sAttrib->zTranslate,
					&tds->bTransform);

		(*s->applyScreenTransform) (s, sAttrib, output, &mTransform);
		(*s->enableOutputClipping) (s, &mTransform, region, output);

		transformToScreenSpace (s, output, -sAttrib->zTranslate,
					&mTransform);

		if ((s->windowOffsetX != 0 || s->windowOffsetY != 0) &&
		    !windowOnAllViewports (w))
		{
		    getWindowMovementForOffset (w, s->windowOffsetX,
						s->windowOffsetY, &offX, &offY);

		    matrixTranslate (&mTransform, offX, offY, 0);
		    matrixTranslate (&tds->bTransform, offX, offY, 0);

		    newMask |= PAINT_WINDOW_WITH_OFFSET_MASK;
		}

		glPushMatrix ();
		glLoadMatrixf (mTransform.m);

		(*s->paintWindow) (w, &w->paint, &mTransform, &infiniteRegion,
				   newMask);
		glPopMatrix ();

		(*s->disableOutputClipping) (s);
	    }
	}

	tds->painting3D   = FALSE;
	tds->currentScale = tds->basicScale;
    }
}

static Bool
tdPaintOutput (CompScreen              *s,
	       const ScreenPaintAttrib *sAttrib,
	       const CompTransform     *transform,
	       Region                  region,
	       CompOutput              *output,
	       unsigned int            mask)
{
    Bool status;

    TD_SCREEN (s);

    if (tds->active)
    {
	mask |= PAINT_SCREEN_TRANSFORMED_MASK |
	        PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }

    UNWRAP (tds, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (tds, s, paintOutput, tdPaintOutput);

    return status;
}

static void
tdDonePaintScreen (CompScreen *s)
{
    TD_SCREEN (s);

    if (tds->active)
	damageScreen (s);

    UNWRAP (tds, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (tds, s, donePaintScreen, tdDonePaintScreen);
}

static CompWindow*
tdWalkFirst (CompScreen *s)
{
    TD_SCREEN (s);
    return tds->first;
}

static CompWindow*
tdWalkLast (CompScreen *s)
{
    TD_SCREEN (s);
    return tds->last;
}

static CompWindow*
tdWalkNext (CompWindow *w)
{
    TD_WINDOW (w);
    return tdw->next;
}

static CompWindow*
tdWalkPrev (CompWindow *w)
{
    TD_WINDOW (w);
    return tdw->prev;
}

static void
tdInitWindowWalker (CompScreen *s,
		    CompWalker *walker)
{
    TD_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (tds, s, initWindowWalker);
    (*s->initWindowWalker) (s, walker);
    WRAP (tds, s, initWindowWalker, tdInitWindowWalker);

    if (tds->active && cs->paintOrder == BTF && tds->painting3D)
    {
	walker->first = tdWalkFirst;
	walker->last =  tdWalkLast;
	walker->next =  tdWalkNext;
	walker->prev =  tdWalkPrev;
    }
}

static Bool
tdInitDisplay (CompPlugin  *p,
	       CompDisplay *d)
{
    tdDisplay *tdd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    if (!checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    tdd = malloc (sizeof (tdDisplay));
    if (!tdd)
	return FALSE;

    tdd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (tdd->screenPrivateIndex < 0)
    {
	free (tdd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = tdd;

    return TRUE;
}

static void
tdFiniDisplay (CompPlugin  *p,
	       CompDisplay *d)
{
    TD_DISPLAY (d);

    freeScreenPrivateIndex (d, tdd->screenPrivateIndex);

    free (tdd);
}

static Bool
tdInitScreen (CompPlugin *p,
	      CompScreen *s)
{
    tdScreen *tds;

    TD_DISPLAY (s->display);
    CUBE_SCREEN (s);

    tds = malloc (sizeof (tdScreen));
    if (!tds)
	return FALSE;

    tds->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (tds->windowPrivateIndex < 0)
    {
	free (tds);
	return FALSE;
    }

    tds->basicScale     = 1.0f;
    tds->currentScale   = 1.0f;

    tds->active     = FALSE;
    tds->painting3D = FALSE;

    tds->first = NULL;
    tds->last  = NULL;

    s->base.privates[tdd->screenPrivateIndex].ptr = tds;

    WRAP (tds, s, paintWindow, tdPaintWindow);
    WRAP (tds, s, paintOutput, tdPaintOutput);
    WRAP (tds, s, donePaintScreen, tdDonePaintScreen);
    WRAP (tds, s, preparePaintScreen, tdPreparePaintScreen);
    WRAP (tds, s, initWindowWalker, tdInitWindowWalker);
    WRAP (tds, s, applyScreenTransform, tdApplyScreenTransform);
    WRAP (tds, cs, postPaintViewport, tdPostPaintViewport);

    return TRUE;
}

static void
tdFiniScreen (CompPlugin *p,
	      CompScreen *s)
{
    TD_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (tds, s, paintWindow);
    UNWRAP (tds, s, paintOutput);
    UNWRAP (tds, s, donePaintScreen);
    UNWRAP (tds, s, preparePaintScreen);
    UNWRAP (tds, s, initWindowWalker);
    UNWRAP (tds, s, applyScreenTransform);
    UNWRAP (tds, cs, postPaintViewport);

    freeWindowPrivateIndex (s, tds->windowPrivateIndex);
	
    free (tds);
}

static Bool
tdInitWindow (CompPlugin *p,
	      CompWindow *w)
{
    tdWindow *tdw;

    TD_SCREEN (w->screen);

    tdw = malloc (sizeof (tdWindow));
    if (!tdw)
	return FALSE;

    tdw->is3D  = FALSE;
    tdw->prev  = NULL;
    tdw->next  = NULL;
    tdw->depth = 0.0f;

    w->base.privates[tds->windowPrivateIndex].ptr = tdw;

    return TRUE;
}

static void
tdFiniWindow (CompPlugin *p,
	      CompWindow *w)
{
    TD_WINDOW (w);

    free (tdw);
}

static Bool
tdInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
tdFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
tdInitObject (CompPlugin *p,
	      CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) tdInitDisplay,
	(InitPluginObjectProc) tdInitScreen,
	(InitPluginObjectProc) tdInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
tdFiniObject (CompPlugin *p,
	      CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) tdFiniDisplay,
	(FiniPluginObjectProc) tdFiniScreen,
	(FiniPluginObjectProc) tdFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompPluginVTable tdVTable = {
    "3d",
    0,
    tdInit,
    tdFini,
    tdInitObject,
    tdFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &tdVTable;
}

