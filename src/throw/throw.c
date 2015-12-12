/**
 *
 * Compiz throw windows plugin
 *
 * throw.c
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
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
#include <string.h>
#include <math.h>

#include <compiz-core.h>

#include "throw_options.h"

#define PI 3.1415926

static int displayPrivateIndex;

typedef struct _ThrowDisplay
{
    int screenPrivateIndex;
} ThrowDisplay;

typedef struct _ThrowScreen
{
    WindowGrabNotifyProc   windowGrabNotify;
    WindowUngrabNotifyProc windowUngrabNotify;
    WindowMoveNotifyProc   windowMoveNotify;
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc	   donePaintScreen;

    int                    windowPrivateIndex;
} ThrowScreen;


typedef struct _ThrowWindow
{
    float		   xVelocity;
    float		   yVelocity;
    int			   time;
    int			   lastdx;
    int			   lastdy;
    int			   totaldx;
    int			   totaldy;
    Bool		   moving;
} ThrowWindow;

#define GET_THROW_DISPLAY(d)				    \
    ((ThrowDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define THROW_DISPLAY(d)			  \
    ThrowDisplay *td = GET_THROW_DISPLAY (d)

#define GET_THROW_SCREEN(s, td)					\
    ((ThrowScreen *) (s)->base.privates[(td)->screenPrivateIndex].ptr)

#define THROW_SCREEN(s)						       \
    ThrowScreen *ts = GET_THROW_SCREEN (s, GET_THROW_DISPLAY (s->display))

#define GET_THROW_WINDOW(w, ts)                                        \
    ((ThrowWindow *) (w)->base.privates[(ts)->windowPrivateIndex].ptr)

#define THROW_WINDOW(w)                                         \
    ThrowWindow *tw = GET_THROW_WINDOW  (w,                    \
                       GET_THROW_SCREEN  (w->screen,            \
                       GET_THROW_DISPLAY (w->screen->display)))

#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)

/*  Handle the velocity */
static void
throwPreparePaintScreen (CompScreen *s,
			int        ms)
{
    CompWindow *w;
    THROW_SCREEN (s);

    for (w = s->windows; w; w = w->next)
    {
	THROW_WINDOW (w);

	if (tw->moving)
	    tw->time += ms;

        tw->xVelocity /= (1.0 + (throwGetFrictionConstant (s) / 100));
	tw->yVelocity /= (1.0 + (throwGetFrictionConstant (s) / 100));

	if (!tw->moving && (
	    (tw->xVelocity < 0.0f || tw->xVelocity > 0.0f) ||
	    (tw->yVelocity < 0.0f || tw->yVelocity > 0.0)))
	{
	    int dx = roundf(tw->xVelocity * (ms / 10) * (throwGetVelocityX (s) / 10));
	    int dy = roundf (tw->yVelocity * (ms / 10) * (throwGetVelocityY (s) / 10));

	    if (throwGetConstrainX (s))
	    {
		if ((WIN_REAL_X (w) + dx) < 0)
		    dx = 0;
		else if ((WIN_REAL_X (w) + WIN_REAL_W (w) + dx) > w->screen->width)
		    dx = 0;
	    }
	    if (throwGetConstrainY (s))
	    {
		if ((WIN_REAL_Y (w) + dy) < 0)
		    dy = 0;
		else if ((WIN_REAL_Y (w) + WIN_REAL_H (w) + dy) > w->screen->height)
		    dy = 0;
	    }

	    moveWindow (w, dx, dy, TRUE, FALSE);
	    syncWindowPosition (w);
	}

    }

    UNWRAP (ts, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (ts, s, preparePaintScreen, throwPreparePaintScreen);
}

static void
throwDonePaintScreen (CompScreen *s)
{
    THROW_SCREEN (s);
    CompWindow *w;
    for (w = s->windows; w; w = w->next)
    {
	THROW_WINDOW (w);
	if (tw->moving ||
	    (tw->xVelocity < 0.0f || tw->xVelocity > 0.0f) ||
	    (tw->yVelocity < 0.0f || tw->yVelocity > 0.0f))
	{
	    addWindowDamage (w);
	}
    }

    UNWRAP (ts, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ts, s, donePaintScreen, throwDonePaintScreen);
}

static void
throwWindowGrabNotify (CompWindow   *w,
		      int          x,
		      int          y,
		      unsigned int state,
		      unsigned int mask)
{
    CompScreen * s = w->screen;

    THROW_SCREEN (s);

    if (mask & CompWindowGrabMoveMask)
    {
	THROW_WINDOW (w);

	tw->moving = TRUE;

	tw->time = 0;
	tw->lastdx = 0;
	tw->lastdy = 0;
	tw->totaldx = 0;
	tw->totaldy = 0;
	tw->xVelocity = 0.0f;
	tw->yVelocity = 0.0f;
    }

    UNWRAP (ts, s, windowGrabNotify);
    (*s->windowGrabNotify) (w, x, y, state, mask);
    WRAP (ts, s, windowGrabNotify, throwWindowGrabNotify);
}

static void
throwWindowUngrabNotify (CompWindow *w)
{
    CompScreen *s = w->screen;

    THROW_SCREEN (s);
    THROW_WINDOW (w);

    tw->moving = FALSE;

    UNWRAP (ts, s, windowUngrabNotify);
    (*s->windowUngrabNotify) (w);
    WRAP (ts, s, windowUngrabNotify, throwWindowUngrabNotify);
}

static void
throwWindowMoveNotify (CompWindow *w,
		       int        dx,
		       int        dy,
		       Bool	  immediate)
{
    THROW_WINDOW (w);
    THROW_SCREEN (w->screen);

    if (tw->moving)
    {

	    if (( tw->lastdx < 0 && dx > 0 ) ||
		( tw->lastdx > 0 && dx < 0 ))
	    {
		tw->time = 1;
		tw->totaldx = 0;
		tw->xVelocity = 0.0f;
	    }
	    if (( tw->lastdy < -3 && dy > 3 ) ||
		( tw->lastdy > 3 && dy < -3 ))
	    {
		tw->time = 1;
		tw->totaldy = 0;
		tw->yVelocity = 0.0f;
	    }
	    tw->totaldx += dx;
	    tw->totaldy += dy;

	    tw->xVelocity = (tw->totaldx) / ((float) tw->time / 50);
	    tw->yVelocity = (tw->totaldy) / ((float) tw->time / 50);
    }

    tw->lastdx = dx;
    tw->lastdy = dy;

    UNWRAP (ts, w->screen, windowMoveNotify);
    (*w->screen->windowMoveNotify) (w, dx, dy, immediate);
    WRAP (ts, w->screen, windowMoveNotify, throwWindowMoveNotify);
}

static Bool
throwInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    ThrowDisplay *td;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    td = malloc (sizeof (ThrowDisplay));
    if (!td)
	return FALSE;

    td->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (td->screenPrivateIndex < 0)
    {
	free (td);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = td;

    return TRUE;
}

static void
throwFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    THROW_DISPLAY (d);

    freeScreenPrivateIndex (d, td->screenPrivateIndex);

    free (td);
}

static Bool
throwInitScreen (CompPlugin *p,
		CompScreen *s)
{
    ThrowScreen * ts;

    THROW_DISPLAY (s->display);

    ts = malloc (sizeof (ThrowScreen));
    if (!ts)
	return FALSE;

    ts->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ts->windowPrivateIndex < 0)
    {
	free (ts);
	return FALSE;
    }

    WRAP (ts, s, windowGrabNotify, throwWindowGrabNotify);
    WRAP (ts, s, windowUngrabNotify, throwWindowUngrabNotify);
    WRAP (ts, s, preparePaintScreen, throwPreparePaintScreen);
    WRAP (ts, s, windowMoveNotify, throwWindowMoveNotify);
    WRAP (ts, s, donePaintScreen, throwDonePaintScreen);

    s->base.privates[td->screenPrivateIndex].ptr = ts;

    return TRUE;
}

static Bool
throwInitWindow (CompPlugin *p,
		 CompWindow *w)
{
    ThrowWindow *tw;

    THROW_SCREEN (w->screen);

    tw = calloc (1, sizeof (ThrowWindow));
    if (!tw)
	return FALSE;

    tw->xVelocity = 0.0f;
    tw->yVelocity = 0.0f;
    tw->lastdx = 0;
    tw->lastdy = 0;
    tw->totaldx = 0;
    tw->totaldy = 0;
    tw->moving = FALSE;
    tw->time = 0;

    w->base.privates[ts->windowPrivateIndex].ptr = tw;

    return TRUE;
}

static void
throwFiniWindow (CompPlugin *p,
		 CompWindow *w)
{
    THROW_WINDOW (w);
    free (tw);
}

static void
throwFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    THROW_SCREEN (s);

    freeWindowPrivateIndex (s, ts->windowPrivateIndex);

    UNWRAP (ts, s, windowGrabNotify);
    UNWRAP (ts, s, windowUngrabNotify);
    UNWRAP (ts, s, preparePaintScreen);
    UNWRAP (ts, s, windowMoveNotify);
    UNWRAP (ts, s, donePaintScreen);

    free (ts);
}

static CompBool
throwInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) throwInitDisplay,
	(InitPluginObjectProc) throwInitScreen,
	(InitPluginObjectProc) throwInitWindow,
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
throwFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) throwFiniDisplay,
	(FiniPluginObjectProc) throwFiniScreen,
	(FiniPluginObjectProc) throwFiniWindow,
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
throwInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
throwFini (CompPlugin *p)
{
    freeDisplayPrivateIndex(displayPrivateIndex);
}

static CompPluginVTable throwVTable = {
    "throw",
    0,
    throwInit,
    throwFini,
    throwInitObject,
    throwFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &throwVTable;
}
