/**
 *
 * ghost.c
 *
 * Copyright (c) 2008 Douglas Young <rcxdude@gmail.com>
 * Input shaping adapted from FreeWins plugin
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
 **/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <X11/extensions/shape.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>

#include "ghost_options.h"

static int displayPrivateIndex;

typedef struct _GhostDisplay
{
    int screenPrivateIndex;
    MousePollFunc *mpFunc;
    CompWindow *lastActive;
    Bool active;
} GhostDisplay;

#define ANIMATE_UP 1
#define ANIMATE_DOWN 2
#define ANIMATE_NONE 0

typedef struct _GhostWindow
{
    Bool  isghost;
    Bool  selected;
    Bool  faded;
    int   animate;
    float opacity;
    float saturation;
    float brightness;

    XRectangle *inputRects;
    int        nInputRects;
    int        inputRectOrdering;
    XRectangle *frameInputRects;
    int        frameNInputRects;
    int        frameInputRectOrdering;
} GhostWindow;

typedef struct _GhostScreen
{
    int windowPrivateIndex;

    PositionPollingHandle  pollHandle;
    PaintWindowProc paintWindow;
    PreparePaintScreenProc preparePaintScreen;
    int active;
} GhostScreen;


#define GET_GHOST_DISPLAY(d) \
    ((GhostDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define GHOST_DISPLAY(d) \
    GhostDisplay *gd = GET_GHOST_DISPLAY (d)

#define GET_GHOST_SCREEN(s, gd) \
    ((GhostScreen *) (s)->base.privates[(gd)->screenPrivateIndex].ptr)

#define GHOST_SCREEN(s) \
    GhostScreen *gs = GET_GHOST_SCREEN (s, \
		      GET_GHOST_DISPLAY (s->display))

#define GET_GHOST_WINDOW(w, gs) \
    ((GhostWindow *) (w)->base.privates[(gs)->windowPrivateIndex].ptr)

#define GHOST_WINDOW(w) \
    GhostWindow *gw = GET_GHOST_WINDOW  (w, \
		      GET_GHOST_SCREEN  (w->screen, \
		      GET_GHOST_DISPLAY (w->screen->display)))

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define SPEED (ghostGetSpeed(s->display) * 0.0005f)

static Bool
ghostEnableWindow (CompWindow *w)
{
    GHOST_WINDOW (w);
    if (gw->isghost)
	return FALSE;
    CompWindow *fw;
    Display    *dpy = w->screen->display->display;
    XRectangle *rects;
    int        count = 0, ordering;

    rects = XShapeGetRectangles (dpy, w->id, ShapeInput, &count, &ordering);

    /* check if the returned shape exactly matches the window shape -
       if that is true, the window currently has no set input shape */
    if ((count == 1) &&
	(rects[0].x == -w->serverBorderWidth) &&
	(rects[0].y == -w->serverBorderWidth) &&
	(rects[0].width == (w->serverWidth + w->serverBorderWidth)) &&
	(rects[0].height == (w->serverHeight + w->serverBorderWidth)))
    {
	count = 0;
    }

    /* save old shape */
    gw->inputRects = rects;
    gw->nInputRects = count;
    gw->inputRectOrdering = ordering;

    fw = findWindowAtDisplay (w->screen->display, w->frame);
    if (fw)
    {
	rects = XShapeGetRectangles (dpy, fw->id, ShapeInput, &count, &ordering);

	/* check if the returned shape exactly matches the window shape -
	   if that is true, the window currently has no set input shape */
	if ((count == 1) &&
	    (rects[0].x == -fw->serverBorderWidth) &&
	    (rects[0].y == -fw->serverBorderWidth) &&
	    (rects[0].width == (fw->serverWidth + fw->serverBorderWidth)) &&
	    (rects[0].height == (fw->serverHeight + fw->serverBorderWidth)))
	{
	    count = 0;
	}

	/* save old shape */
	gw->frameInputRects = rects;
	gw->frameNInputRects = count;
	gw->frameInputRectOrdering = ordering;
    } else
    {
	gw->frameInputRects        = NULL;
	gw->frameNInputRects       = -1;
	gw->frameInputRectOrdering = 0;
    }

    /* clear shape */
    XShapeSelectInput (dpy, w->id, NoEventMask);
    XShapeCombineRectangles  (dpy, w->id, ShapeInput, 0, 0,
			      NULL, 0, ShapeSet, 0);

    if (w->frame)
	XShapeCombineRectangles  (dpy, w->frame, ShapeInput, 0, 0,
				  NULL, 0, ShapeSet, 0);

    XShapeSelectInput (dpy, w->id, ShapeNotify);

    int x, y;
    GHOST_DISPLAY(w->screen->display);
    (*gd->mpFunc->getCurrentPosition) (w->screen, &x, &y);
    gw->isghost = TRUE;
    if (!ghostGetFadeOnMouseover (w->screen->display) ||
	  (x > WIN_X(w) &&
	   x < WIN_X(w) + WIN_W(w) &&
	   y > WIN_Y(w) &&
	   y < WIN_Y(w) + WIN_H(w)))

    {
	gw->faded   = TRUE;
	gw->animate = ANIMATE_DOWN;
    }
    return TRUE;
}

static Bool
ghostDisableWindow (CompWindow *w)
{
    GHOST_WINDOW (w);
    if (!gw->isghost)
	return FALSE;

    Display *dpy = w->screen->display->display;

    if (gw->nInputRects)
    {
	XShapeCombineRectangles (dpy, w->id, ShapeInput, 0, 0,
				 gw->inputRects, gw->nInputRects,
				 ShapeSet, gw->inputRectOrdering);
    }
    else
    {
	XShapeCombineMask (dpy, w->id, ShapeInput, 0, 0, None, ShapeSet);
    }

    if (gw->frameNInputRects >= 0)
    {
	if (gw->frameInputRects)
	{
	    XShapeCombineRectangles (dpy, w->frame, ShapeInput, 0, 0,
				     gw->frameInputRects,
				     gw->frameNInputRects,
				     ShapeSet,
				     gw->frameInputRectOrdering);
	}
	else
	{
	    XShapeCombineMask (dpy, w->frame, ShapeInput, 0, 0, None, ShapeSet);
	}
    }

    int x, y;
    GHOST_DISPLAY(w->screen->display);
    (*gd->mpFunc->getCurrentPosition) (w->screen, &x, &y);
    gw->isghost = FALSE;
    if (!ghostGetFadeOnMouseover (w->screen->display) ||
	  (x > WIN_X(w) &&
	   x < WIN_X(w) + WIN_W(w) &&
	   y > WIN_Y(w) &&
	   y < WIN_Y(w) + WIN_H(w)))
    {
	gw->faded   = FALSE;
	gw->animate = ANIMATE_UP;
    }
    return TRUE;
}

static Bool
ghostToggle (CompDisplay    *d,
	     CompAction     *action,
	     CompActionState state,
	     CompOption     *option,
	     int           nOption)
{
    CompScreen *s;
    GHOST_DISPLAY (d);
    if (gd->active)
    {
	for (s = d->screens; s; s = s->next)
	{
	    CompWindow *w;
	    for (w = s->windows; w; w = w->next)
	    {
		GHOST_WINDOW (w);
		if((matchEval(ghostGetWindowTypes (s->display),w)) &&
		   !(w->invisible || w->destroyed || w->hidden || w->minimized))
		{
		    ghostDisableWindow (w);
		    gw->selected = FALSE;
		}
	    }
	}
	gd->active=FALSE;
    } else {
	for (s = d->screens; s; s = s->next)
	{
	    CompWindow *w;
	    for (w = s->windows; w; w = w->next)
	    {
		GHOST_WINDOW (w);
		if((matchEval(ghostGetWindowTypes (s->display),w)) &&
		   !(w->invisible || w->destroyed || w->hidden || w->minimized))
		{
		    if (w->id != d->activeWindow || ghostGetGhostActive(d))
			ghostEnableWindow (w);
		    gw->selected = TRUE;
		}
	    }
	}
	gd->active=TRUE;
    }
    return TRUE;
}
static Bool
ghostToggleWindow  (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
    CompWindow *w;
    int xid;
    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    GHOST_WINDOW (w);
    if (gw->selected)
    {
	ghostDisableWindow (w);
	gw->selected = FALSE;
    } else {
	if (w->id != d->activeWindow || ghostGetGhostActive(d))
	    ghostEnableWindow (w);
	gw->selected = TRUE;
    }
    return TRUE;
}


static Bool
ghostPaintWindow (CompWindow              *w,
		  const WindowPaintAttrib *attrib,
		  const CompTransform     *transform,
		  Region                  region,
		  unsigned int            mask)
{
    GHOST_SCREEN(w->screen);
    GHOST_WINDOW(w);
    int status;
    if (gw->faded || gw->animate)
    {
	WindowPaintAttrib wAttrib = *attrib;
	wAttrib.opacity *= gw->opacity/100;
	wAttrib.saturation *= gw->saturation/100;
	wAttrib.brightness *= gw->brightness/100;
	if (gw->animate)
	    addWindowDamage (w);
	UNWRAP(gs,w->screen, paintWindow);
	status = (*w->screen->paintWindow)(w, &wAttrib, transform, region, mask);
	WRAP(gs, w->screen, paintWindow, ghostPaintWindow);
    } else {
	UNWRAP(gs,w->screen, paintWindow);
	status = (*w->screen->paintWindow)(w, attrib, transform, region, mask);
	WRAP(gs, w->screen, paintWindow, ghostPaintWindow);
    }
    return status;
}


static void
ghostPreparePaintScreen (CompScreen *s,
			 int	ms)
{
    GHOST_SCREEN(s);
    GHOST_DISPLAY(s->display);
    CompWindow *w;
    float topacity = ghostGetOpacity    (s->display);
    float tsat     = ghostGetSaturation (s->display);
    float tbright  = ghostGetBrightness(s->display);
    if (s->display->activeWindow != (gd->lastActive ? gd->lastActive->id : 0)
	&& !ghostGetGhostActive(s->display) )
    {
	w = findWindowAtDisplay (s->display, s->display->activeWindow);
	if (w)
	{
	    GhostWindow *active = GET_GHOST_WINDOW  (w, \
				  GET_GHOST_SCREEN  (w->screen, \
				  GET_GHOST_DISPLAY (w->screen->display)));
	    if (gd->lastActive)
	    {
		GhostWindow *last   = GET_GHOST_WINDOW  (gd->lastActive, \
				      GET_GHOST_SCREEN  (gd->lastActive->screen, \
				      GET_GHOST_DISPLAY (gd->lastActive->screen->display)));
		if (last->selected)
		    ghostEnableWindow(gd->lastActive);
	    }
	    if (active->selected)
		ghostDisableWindow(w);
	    gd->lastActive = w;
	}
    }

    for (w = s->windows; w; w = w->next)
    {
	GHOST_WINDOW(w);
	if (gw->animate != ANIMATE_NONE)
	{
	    if (gw->animate == ANIMATE_DOWN)
	    {
		gw->opacity    = fmax(gw->opacity    - (gw->opacity    - topacity) * ms * SPEED,topacity);
		gw->saturation = fmax(gw->saturation - (gw->saturation - tsat)     * ms * SPEED,tsat    );
		gw->brightness = fmax(gw->brightness - (gw->brightness - tbright)  * ms * SPEED,tbright );
		if (gw->opacity    <= topacity + 1 &&
		    gw->saturation <= tsat     + 1 &&
		    gw->brightness <= tbright  + 1)
		    gw->animate = ANIMATE_NONE;
	    }
	    else if (gw->faded == TRUE)
	    {
		gw->opacity    = fmin(gw->opacity    + (topacity - gw->opacity   ) * ms * SPEED,topacity);
		gw->saturation = fmin(gw->saturation + (tsat     - gw->saturation) * ms * SPEED,tsat    );
		gw->brightness = fmin(gw->brightness + (tbright  - gw->brightness) * ms * SPEED,tbright );
		if (gw->opacity    >= topacity - 1 &&
		    gw->saturation >= tsat     - 1 &&
		    gw->brightness >= tbright  - 1)
		    gw->animate = ANIMATE_NONE;
	    }
	    else
	    {
		gw->opacity    = fmin(gw->opacity    + (100 - gw->opacity   ) * ms * SPEED,100);
		gw->saturation = fmin(gw->saturation + (100 - gw->saturation) * ms * SPEED,100);
		gw->brightness = fmin(gw->brightness + (100 - gw->brightness) * ms * SPEED,100);
		if (gw->opacity    >= 99 &&
		    gw->saturation >= 99 &&
		    gw->brightness >= 99)
		    gw->animate = ANIMATE_NONE;
	    }
	}
    }
    UNWRAP(gs, s, preparePaintScreen);
    (*s->preparePaintScreen)(s, ms);
    WRAP(gs, s, preparePaintScreen, ghostPreparePaintScreen);
}

static void
positionUpdate (CompScreen *s, int x, int y)
{
    CompWindow *w;
    if (!ghostGetFadeOnMouseover(s->display))
	return;
    if (otherScreenGrabExist(s, "ghost", 0))
	return;
    for (w = s->windows; w; w = w->next)
    {
	GHOST_WINDOW (w);
	if(gw->isghost &&
	   x > WIN_X(w) &&
	   x < WIN_X(w) + WIN_W(w) &&
	   y > WIN_Y(w) &&
	   y < WIN_Y(w) + WIN_H(w))
	{
	    gw->faded = TRUE;
	    gw->animate = ANIMATE_DOWN;
	    addWindowDamage (w);
	} else if (gw->faded) {
	    gw->faded = FALSE;
	    gw->animate = ANIMATE_UP;
	    addWindowDamage (w);
	}
    }
}

static void
ghostAppChange     (CompDisplay *d,
		    CompOption *opt,
		    GhostDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	CompWindow *w;
	for (w = s->windows; w; w = w->next)
	{
	    GHOST_WINDOW(w);
	    if (gw->faded && (ghostGetOpacity(d)    > gw->opacity    ||
		              ghostGetSaturation(d) > gw->saturation ||
			      ghostGetBrightness(d) > gw->brightness ))
		gw->animate = ANIMATE_UP;
	    else if (gw->faded && (ghostGetOpacity(d)    < gw->opacity   ||
		                   ghostGetSaturation(d) < gw->saturation ||
				   ghostGetBrightness(d) < gw->brightness ))
		gw->animate = ANIMATE_DOWN;
	    addWindowDamage(w);
	}
    }
}


static Bool
ghostInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    GhostDisplay *gd;
    int mousepollindex;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    gd = malloc (sizeof (GhostDisplay));
    if (!gd)
	return FALSE;

    gd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (gd->screenPrivateIndex < 0)
    {
	free (gd);
	return FALSE;
    }
    if (!getPluginDisplayIndex (d, "mousepoll", &mousepollindex))
	return FALSE;

    d->base.privates[displayPrivateIndex].ptr = gd;
    ghostSetGhostToggleKeyInitiate(d,ghostToggle);
    ghostSetGhostToggleButtonInitiate(d,ghostToggle);
    ghostSetGhostToggleWindowKeyInitiate(d,ghostToggleWindow);
    ghostSetGhostToggleWindowButtonInitiate(d,ghostToggleWindow);
    ghostSetSaturationNotify(d,ghostAppChange);
    ghostSetOpacityNotify(d,ghostAppChange);
    ghostSetBrightnessNotify(d,ghostAppChange);

    gd->mpFunc = d->base.privates[mousepollindex].ptr;
    gd->active = FALSE;
    gd->lastActive = findWindowAtDisplay (d, d->activeWindow);

    return TRUE;
}

static void
ghostFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    GHOST_DISPLAY (d);
    freeScreenPrivateIndex (d, gd->screenPrivateIndex);
    free (gd);
}

static Bool
ghostInitScreen (CompPlugin *p,
		CompScreen *s)
{
    GhostScreen *gs;

    GHOST_DISPLAY(s->display);

    gs = malloc (sizeof (GhostScreen));
    if (!gs)
	return FALSE;

    WRAP(gs, s, preparePaintScreen, ghostPreparePaintScreen);
    WRAP(gs, s, paintWindow, ghostPaintWindow);
    gs->windowPrivateIndex = allocateWindowPrivateIndex (s);
    s->base.privates[gd->screenPrivateIndex].ptr = gs;

    gs->pollHandle = (*gd->mpFunc->addPositionPolling) (s, positionUpdate);

    return TRUE;
}

static void
ghostFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    GHOST_SCREEN (s);
    GHOST_DISPLAY (s->display);
    UNWRAP(gs, s, preparePaintScreen);
    UNWRAP(gs, s, paintWindow);
    (*gd->mpFunc->removePositionPolling) (s, gs->pollHandle);
    free (gs);
}

static Bool
ghostInitWindow (CompPlugin *p,
		 CompWindow *w)
{
    GhostWindow *gw;

    GHOST_SCREEN (w->screen);

    gw = malloc (sizeof (GhostWindow));
    if (!gw)
	return FALSE;
    w->base.privates[gs->windowPrivateIndex].ptr = gw;
    gw->isghost = FALSE;
    gw->selected = FALSE;
    gw->animate = ANIMATE_NONE;
    gw->faded = FALSE;
    gw->opacity = 100;
    gw->saturation = 100;
    gw->brightness = 100;

    return TRUE;
}

static void
ghostFiniWindow (CompPlugin *p,
		 CompWindow *w)
{
    GHOST_WINDOW (w);
    GHOST_DISPLAY (w->screen->display);
    ghostDisableWindow (w);
    if (w == gd->lastActive)
	gd->lastActive = 0;
    free (gw);
}

static CompBool
ghostInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) ghostInitDisplay,
	(InitPluginObjectProc) ghostInitScreen,
	(InitPluginObjectProc) ghostInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
ghostFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) ghostFiniDisplay,
	(FiniPluginObjectProc) ghostFiniScreen,
	(FiniPluginObjectProc) ghostFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
ghostInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
ghostFini (CompPlugin *p)
{
    freeDisplayPrivateIndex(displayPrivateIndex);
}

static CompPluginVTable ghostVTable = {
    "ghost",
    0,
    ghostInit,
    ghostFini,
    ghostInitObject,
    ghostFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &ghostVTable;
}
