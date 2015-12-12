/**
 *
 * dialog.c
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

#include "dialog_options.h"

static int displayPrivateIndex;

typedef struct _DialogDisplay
{
    int screenPrivateIndex;
    HandleEventProc handleEvent;
} DialogDisplay;

#define ANIMATE_UP 1
#define ANIMATE_DOWN 2
#define ANIMATE_NONE 0

typedef struct _DialogWindow
{
    int   animate;
    Bool  faded;
    float opacity;
    float saturation;
    float brightness;

    Window oldTransient;
} DialogWindow;

typedef struct _DialogScreen
{
    int windowPrivateIndex;

    PaintWindowProc paintWindow;
    PreparePaintScreenProc preparePaintScreen;
} DialogScreen;


#define GET_DIALOG_DISPLAY(d) \
    ((DialogDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define DIALOG_DISPLAY(d) \
    DialogDisplay *dd = GET_DIALOG_DISPLAY (d)

#define GET_DIALOG_SCREEN(s, dd) \
    ((DialogScreen *) (s)->base.privates[(dd)->screenPrivateIndex].ptr)

#define DIALOG_SCREEN(s) \
    DialogScreen *ds = GET_DIALOG_SCREEN (s, \
		      GET_DIALOG_DISPLAY (s->display))

#define GET_DIALOG_WINDOW(w, ds) \
    ((DialogWindow *) (w)->base.privates[(ds)->windowPrivateIndex].ptr)

#define DIALOG_WINDOW(w) \
    DialogWindow *dw = GET_DIALOG_WINDOW  (w, \
		      GET_DIALOG_SCREEN  (w->screen, \
		      GET_DIALOG_DISPLAY (w->screen->display)))

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define SPEED (dialogGetSpeed(s->display) * 0.0005f)

static Bool
dialogPaintWindow (CompWindow              *w,
		  const WindowPaintAttrib *attrib,
		  const CompTransform     *transform,
		  Region                  region,
		  unsigned int            mask)
{
    DIALOG_SCREEN(w->screen);
    DIALOG_WINDOW(w);
    int status;
    if (dw->faded || dw->animate)
    {
	WindowPaintAttrib wAttrib = *attrib;
	wAttrib.opacity *= dw->opacity/100;
	wAttrib.saturation *= dw->saturation/100;
	wAttrib.brightness *= dw->brightness/100;
	if (dw->animate)
	    addWindowDamage (w);
	UNWRAP(ds,w->screen, paintWindow);
	status = (*w->screen->paintWindow)(w, &wAttrib, transform, region, mask);
	WRAP(ds, w->screen, paintWindow, dialogPaintWindow);
    } else {
	UNWRAP(ds,w->screen, paintWindow);
	status = (*w->screen->paintWindow)(w, attrib, transform, region, mask);
	WRAP(ds, w->screen, paintWindow, dialogPaintWindow);
    }
    return status;
}


static void
dialogPreparePaintScreen (CompScreen *s,
			 int	ms)
{
    DIALOG_SCREEN(s);
    CompWindow *w;
    float topacity = dialogGetOpacity   (s->display);
    float tsat     = dialogGetSaturation(s->display);
    float tbright  = dialogGetBrightness(s->display);
    for (w = s->windows; w; w = w->next)
    {
	DIALOG_WINDOW(w);
	if (dw->animate != ANIMATE_NONE)
	{
	    if (dw->animate == ANIMATE_DOWN)
	    {
		dw->opacity    = fmax(dw->opacity    - (dw->opacity    - topacity) * ms * SPEED,topacity);
		dw->saturation = fmax(dw->saturation - (dw->saturation - tsat)     * ms * SPEED,tsat    );
		dw->brightness = fmax(dw->brightness - (dw->brightness - tbright)  * ms * SPEED,tbright );
		if (dw->opacity    <= topacity + 1 &&
		    dw->saturation <= tsat     + 1 &&
		    dw->brightness <= tbright  + 1)
		    dw->animate = ANIMATE_NONE;
	    }
	    else if (dw->faded == TRUE)
	    {
		dw->opacity    = fmin(dw->opacity    + (topacity - dw->opacity   ) * ms * SPEED,topacity);
		dw->saturation = fmin(dw->saturation + (tsat     - dw->saturation) * ms * SPEED,tsat    );
		dw->brightness = fmin(dw->brightness + (tbright  - dw->brightness) * ms * SPEED,tbright );
		if (dw->opacity    >= topacity - 1 &&
		    dw->saturation >= tsat     - 1 &&
		    dw->brightness >= tbright  - 1)
		    dw->animate = ANIMATE_NONE;
	    }
	    else
	    {
		dw->opacity    = fmin(dw->opacity    + (100 - dw->opacity   ) * ms * SPEED,100);
		dw->saturation = fmin(dw->saturation + (100 - dw->saturation) * ms * SPEED,100);
		dw->brightness = fmin(dw->brightness + (100 - dw->brightness) * ms * SPEED,100);
		if (dw->opacity    >= 99 &&
		    dw->saturation >= 99 &&
		    dw->brightness >= 99)
		    dw->animate = ANIMATE_NONE;
	    }
	}
    }
    UNWRAP(ds, s, preparePaintScreen);
    (*s->preparePaintScreen)(s, ms);
    WRAP(ds, s, preparePaintScreen, dialogPreparePaintScreen);
}

static void
dialogHandleEvent (CompDisplay *d,
		   XEvent *event)
{
    DIALOG_DISPLAY (d);
    if (event->type == UnmapNotify)
    {
	CompWindow *w = findWindowAtDisplay (d, event->xunmap.window);
	if (w && w->transientFor)
	{
	    CompWindow   *ww  = findWindowAtDisplay (w->screen->display, w->transientFor);
	    if (ww)
	    {
		DialogWindow *dww = GET_DIALOG_WINDOW  (ww, \
		                GET_DIALOG_SCREEN  (ww->screen, \
		                GET_DIALOG_DISPLAY (ww->screen->display)));
		if (dww->faded)
		{
		    CompWindow *www;
		    int othertrans = 0;
		    for (www = w->screen->windows; www; www = www->next)
		    {
			if (www->transientFor == ww->id && w->id != www->id &&
			    matchEval(dialogGetDialogtypes (www->screen->display),www))
			    othertrans++;
		    }
		    if (!othertrans)
		    {
			dww->faded = FALSE;
			dww->animate = ANIMATE_UP;
			addWindowDamage (ww);
		    }
		}
	    }
	}
    }
    if (event->type == MapNotify)
    {
	CompWindow *w = findWindowAtDisplay (d, event->xmap.window);
	if (w && w->transientFor && matchEval(dialogGetDialogtypes (w->screen->display),w))
	{
	    CompWindow   *ww  = findWindowAtDisplay (w->screen->display, w->transientFor);
	    DialogWindow *dww = GET_DIALOG_WINDOW  (ww, \
		                GET_DIALOG_SCREEN  (ww->screen, \
		                GET_DIALOG_DISPLAY (ww->screen->display)));
	    if (!dww->faded)
	    {
		dww->faded = TRUE;
		dww->animate = ANIMATE_DOWN;
		addWindowDamage (ww);
	    }
	}
    }
    UNWRAP(dd, d, handleEvent);
    (*d->handleEvent)(d, event);
    WRAP(dd, d, handleEvent, dialogHandleEvent);
}


static void
dialogAppChange     (CompDisplay *d,
		    CompOption *opt,
		    DialogDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	CompWindow *w;
	for (w = s->windows; w; w = w->next)
	{
	    DIALOG_WINDOW(w);
	    if (dw->faded && (dialogGetOpacity(d)    > dw->opacity    ||
		              dialogGetSaturation(d) > dw->saturation ||
			      dialogGetBrightness(d) > dw->brightness ))
		dw->animate = ANIMATE_UP;
	    else if (dw->faded && (dialogGetOpacity(d)    <  dw->opacity   ||
		                   dialogGetSaturation(d) < dw->saturation ||
				   dialogGetBrightness(d) < dw->brightness ))
		dw->animate = ANIMATE_DOWN;
	    addWindowDamage(w);
	}
    }
}


static Bool
dialogInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    DialogDisplay *dd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    dd = malloc (sizeof (DialogDisplay));
    if (!dd)
	return FALSE;

    dd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (dd->screenPrivateIndex < 0)
    {
	free (dd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = dd;
    dialogSetOpacityNotify(d,dialogAppChange);
    dialogSetSaturationNotify(d,dialogAppChange);
    dialogSetBrightnessNotify(d,dialogAppChange);

    WRAP(dd, d, handleEvent, dialogHandleEvent);

    return TRUE;
}

static void
dialogFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    DIALOG_DISPLAY (d);
    freeScreenPrivateIndex (d, dd->screenPrivateIndex);
    UNWRAP(dd, d, handleEvent);
    free (dd);
}

static Bool
dialogInitScreen (CompPlugin *p,
		CompScreen *s)
{
    DialogScreen *ds;
    DIALOG_DISPLAY (s->display);

    ds = malloc (sizeof (DialogScreen));
    if (!ds)
	return FALSE;

    WRAP(ds, s, preparePaintScreen, dialogPreparePaintScreen);
    WRAP(ds, s, paintWindow, dialogPaintWindow);
    ds->windowPrivateIndex = allocateWindowPrivateIndex (s);
    s->base.privates[dd->screenPrivateIndex].ptr = ds;

    return TRUE;
}

static void
dialogFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    DIALOG_SCREEN (s);
    UNWRAP(ds, s, preparePaintScreen);
    UNWRAP(ds, s, paintWindow);
    free (ds);
}

static Bool
dialogInitWindow (CompPlugin *p,
		 CompWindow *w)
{
    DialogWindow *dw;

    DIALOG_SCREEN (w->screen);

    dw = malloc (sizeof (DialogWindow));
    if (!dw)
	return FALSE;
    w->base.privates[ds->windowPrivateIndex].ptr = dw;

    dw->opacity = 100;
    dw->saturation = 100;
    dw->brightness = 100;
    dw->faded = FALSE;

    dw->oldTransient = 0;

    return TRUE;
}

static void
dialogFiniWindow (CompPlugin *p,
		 CompWindow *w)
{
    DIALOG_WINDOW (w);
    DIALOG_SCREEN (w->screen);
    if (w->transientFor)
    {
	CompWindow   *ww  = findWindowAtDisplay (w->screen->display, w->transientFor);
	if (ww)
	{
	    DialogWindow *dww = GET_DIALOG_WINDOW  (ww, \
				GET_DIALOG_SCREEN  (ww->screen, \
				GET_DIALOG_DISPLAY (ww->screen->display)));
	    if (dww && dww->faded)
	    {
		dww->faded = FALSE;
		dww->animate = ANIMATE_UP;
		addWindowDamage (ww);
	    }
	}
    }
    w->base.privates[ds->windowPrivateIndex].ptr = 0;
    free (dw);
}

static CompBool
dialogInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) dialogInitDisplay,
	(InitPluginObjectProc) dialogInitScreen,
	(InitPluginObjectProc) dialogInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
dialogFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) dialogFiniDisplay,
	(FiniPluginObjectProc) dialogFiniScreen,
	(FiniPluginObjectProc) dialogFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
dialogInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
dialogFini (CompPlugin *p)
{
    freeDisplayPrivateIndex(displayPrivateIndex);
}

static CompPluginVTable dialogVTable = {
    "dialog",
    0,
    dialogInit,
    dialogFini,
    dialogInitObject,
    dialogFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &dialogVTable;
}
