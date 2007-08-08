/*
 *
 * Compiz mouse gesture viewport switch plugin
 * mswitch.c
 * Copyright (c) 2007 Robert Carr <racarr@opencompositing.org>
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

#include <compiz.h>
#include <math.h>
#include <X11/Xlib.h>
#include "mswitch_options.h"

static int x,y;
static CompScreen *s;


static void mswitchMove(CompScreen *s, int dx, int dy)
{
	XEvent xev;
	xev.xclient.type = ClientMessage;
	xev.xclient.display = s->display->display;
	xev.xclient.format = 32;
	xev.xclient.message_type = s->display->desktopViewportAtom;
	xev.xclient.window = s->root;
	xev.xclient.data.l[0] = (s->x+dx)*s->width;
	xev.xclient.data.l[1] = (s->y+dy)*s->height;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;
	
	XSendEvent(s->display->display, s->root, FALSE, 
		   SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}


static Bool mswitchBegin(CompDisplay *d, CompAction * action, 
			 CompActionState state, CompOption *option,
			 int nOption)
{
	Window xid;

	xid = getIntOptionNamed(option, nOption, "root", 0);
	s = findScreenAtDisplay(d, xid);

	if (state & CompActionStateInitButton)
		action->state |= CompActionStateTermButton;
	if (state & CompActionStateInitKey)
		action->state |= CompActionStateTermKey;
	
	x = pointerX;
	y = pointerY;
	
	return TRUE;
}

static Bool mswitchTerminate(CompDisplay *d, CompAction * action,
		        CompActionState state, CompOption * option,
			int nOption)
{
	
	int mx = MAX(pointerX,x)-MIN(pointerX,x);
	int my = MAX(pointerY,y)-MIN(pointerY,y);
	
	int dx = 0, dy = 0;

	if (mx > my)
	{
		dx = 1;
		if (my/(float)mx > .75 && my/(float)mx < 1.25)
			dy=1;
	}
	else
	{
		dy = 1;
		if (my/(float)mx > .75 && my/(float)mx < 1.25)
			dx=1;
	}
	

	if (pointerX < x)
		dx *= -1;
	if (pointerY < y)
		dy *= -1;

	mswitchMove(s, dx, dy);
	
	action->state &= ~(CompActionStateTermKey | CompActionStateTermButton);
	
	return FALSE;
}

static Bool mswitchInitDisplay(CompPlugin *p,
			       CompDisplay *d)
{
	mswitchSetBeginInitiate(d, mswitchBegin);
	mswitchSetBeginTerminate(d, mswitchTerminate);
	return TRUE;
}

static int mswitchGetVersion(CompPlugin *p, int version)
{
	return ABIVERSION;
}


CompPluginVTable mswitchVTable = {
	"mswitch",
	mswitchGetVersion,
	0,
	0,
	0,
	mswitchInitDisplay,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

CompPluginVTable * getCompPluginInfo(void)
{
	return &mswitchVTable;
}




