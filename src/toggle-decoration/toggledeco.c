/*
 * Compiz Fusion Toggle-decoration plugin
 *
 * Copyright (c) 2008 Marco Diego Aurélio Mesquita <marcodiegomesquita@gmail.com>
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
 * Author(s):
 * Eduardo Gurgel Pinho <edgurgel@gmail.com>
 * Marco Diego Mesquita <marcodiegomesquita@gmail.com>
 *
 * Description:
 *
 * Toggles decorations of windows on/off.
 *
 */

#include <compiz-core.h>
#include "toggledeco_options.h"

/*
 * Initially triggered keybinding.
 * Fetch the window and toggles its decoration.
 */
static Bool
toggledecoTrigger(CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (w)
    {
	w->mwmDecor = w->mwmDecor ^ MwmDecorAll;
	(*w->screen->windowStateChangeNotify) (w, w->state);
    }

    return TRUE;
}

/*
 * Testing if two windows are on the same viewport
 */

static Bool
toggledecoSameViewport(CompWindow* w1,
		       CompWindow* w2)
{
    int x1,x2,y1,y2;

    defaultViewportForWindow (w1,&x1,&y1);
    defaultViewportForWindow (w2,&x2,&y2);
    return ((x1 == x2) && (y1 == y2));
}

/*
 * Initially triggered keybinding.
 * Fetch the every window and toggles its decoration.
 */
static Bool
toggledecoAllTrigger(CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (w)
    {
	CompScreen *s = w->screen;
	CompWindow *window;
	for (window = s->windows; window; window = window->next)
	{
	    if(!toggledecoSameViewport (window,w)) continue;
	       window->mwmDecor = window->mwmDecor ^ MwmDecorAll;
	       (*window->screen->windowStateChangeNotify) (window, window->state);
	}
    }

    return TRUE;
}

/* Configuration, initialization, boring stuff. --------------------- */
static Bool
toggledecoInitDisplay (CompPlugin  *p,
		       CompDisplay *d)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    toggledecoSetTriggerKeyInitiate (d, toggledecoTrigger);
    toggledecoSetTriggerAllKeyInitiate (d, toggledecoAllTrigger);

    return TRUE;
}

static CompBool
toggledecoInitObject (CompPlugin *p,
		      CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) toggledecoInitDisplay,
	0,
	0
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
toggledecoFiniObject (CompPlugin *p,
		      CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	0,
	0,
	0
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable toggledecoVTable = {
    "toggledeco",
    0,
    0,
    0,
    toggledecoInitObject,
    toggledecoFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &toggledecoVTable;
}
