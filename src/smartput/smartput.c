/*
 * Compiz Fusion Smartput plugin
 *
 * smartput.c
 *
 * Copyright : (C) 2008 by Eduardo Gurgel
 * E-mail    : edgurgel@gmail.com
 *
 * Copyright : (C) 2008 by Marco Diego Aurelio Mesquita
 * E-mail    : marcodiegomesquita@gmail.com
 *
 * Copyright : (C) 2008 by Massimo Mund
 * E-mail    : mo@lancode.de
 *
 * Based on put.c
 * Copyright : (C) 2006 by Darryll Truchan
 * E-mail    : <moppsy@comcast.net>
 *
 * Based on maximumize.c
 * Copyright : (C) 2007 by Kristian Lyngstøl
 * E-mail    : <kristian@bohemians.org>
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
 * Description:
 *
 * Smartput fits a window on an empty space.
 *
 * Todo:
 *  - Comments...
 *  - Animation configuration.
 *  - Smart Put All animation.
 *  - Undo
 *
 * More:
 *  This plugin is based on Maximumize Plugin (Author: Kristian Lyngstøl <kristian@bohemians.org>)
 *  Animation is based on Put Plugin (Author: Darryll Truchan <moopsy@comcast.net>
 *
 */
#include <stdlib.h>
#include <X11/Xatom.h>
#include <math.h>
#include <compiz-core.h>
#include "smartput_options.h"

typedef struct _SmartputUndoInfo {
    Window window;

    int savedX, savedY;
    int savedWidth, savedHeight;
    int newX, newY;
    int newWidth, newHeight;

    unsigned int state;
} SmartputUndoInfo;

static int SmartputDisplayPrivateIndex;

typedef struct _SmartputDisplay
{
    int screenPrivateIndex;

    HandleEventProc handleEvent; /* handle event function pointer */

    Window  lastWindow;

    Atom compizSmartputWindowAtom;    /* client event atom         */
} SmartputDisplay;

typedef struct _SmartputScreen
{
    int windowPrivateIndex;

    PreparePaintScreenProc preparePaintScreen;	/* function pointer         */
    DonePaintScreenProc    donePaintScreen;	/* function pointer         */
    PaintOutputProc        paintOutput;	        /* function pointer         */
    PaintWindowProc        paintWindow;	        /* function pointer         */

    Window  lastWindow;

    int        animation;			/* animation flag           */
    int        grabIndex;			/* screen grab index        */

    SmartputUndoInfo undoInfo;

} SmartputScreen;

typedef struct _SmartputWindow
{
    GLfloat xVelocity, yVelocity;	/* animation velocity       */
    GLfloat tx, ty;			/* animation translation    */

    int lastX, lastY;			/* starting position        */
    int targetX, targetY;               /* target of the animation  */

    Bool animation;			/* animation flag           */

    XWindowChanges *xwc;
    unsigned int   mask;
} SmartputWindow;

#define SMARTPUT_DISPLAY(d) PLUGIN_DISPLAY(d, Smartput, sp)
#define SMARTPUT_SCREEN(s) PLUGIN_SCREEN(s, Smartput, sp)
#define SMARTPUT_WINDOW(w) PLUGIN_WINDOW(w, Smartput, sp)


/* Generates a region containing free space (here the
 * active window counts as free space). The region argument
 * is the start-region (ie: the output dev).
 */
static Region
smartputEmptyRegion (CompWindow *window,
		     Region     region)
{
    CompScreen *s = window->screen;
    CompWindow *w;
    Region     newRegion, tmpRegion;
    XRectangle tmpRect;

    newRegion = XCreateRegion ();
    if (!newRegion)
	return NULL;

    tmpRegion = XCreateRegion ();
    if (!tmpRegion)
    {
	XDestroyRegion (newRegion);
	return NULL;
    }

    XUnionRegion (region, newRegion, newRegion);

    for (w = s->windows; w; w = w->next)
    {
	EMPTY_REGION (tmpRegion);
        if (w->id == window->id)
            continue;

        if (w->invisible || w->hidden || w->minimized)
            continue;

	if (w->wmType & CompWindowTypeDesktopMask)
	    continue;

	if (w->wmType & CompWindowTypeDockMask)
	{
	    if (w->struts)
	    {
		XUnionRectWithRegion (&w->struts->left, tmpRegion, tmpRegion);
		XUnionRectWithRegion (&w->struts->right, tmpRegion, tmpRegion);
		XUnionRectWithRegion (&w->struts->top, tmpRegion, tmpRegion);
		XUnionRectWithRegion (&w->struts->bottom, tmpRegion, tmpRegion);
		XSubtractRegion (newRegion, tmpRegion, newRegion);
	    }
	    continue;
	}

	tmpRect.x = w->serverX - w->input.left;
	tmpRect.y = w->serverY - w->input.top;
	tmpRect.width  = w->serverWidth + w->input.right + w->input.left;
	tmpRect.height = w->serverHeight + w->input.top +
	                 w->input.bottom;
	XUnionRectWithRegion (&tmpRect, tmpRegion, tmpRegion);
	XSubtractRegion (newRegion, tmpRegion, newRegion);
    }

    XDestroyRegion (tmpRegion);

    return newRegion;
}

/* Returns true if box a has a larger area than box b.
 */
static Bool
smartputBoxCompare (BOX a,
		    BOX b)
{
    int areaA, areaB;

    areaA = (a.x2 - a.x1) * (a.y2 - a.y1);
    areaB = (b.x2 - b.x1) * (b.y2 - b.y1);

    return (areaA > areaB);
}

/* Extends the given box for Window w to fit as much space in region r.
 * If XFirst is true, it will first expand in the X direction,
 * then Y. This is because it gives different results.
 * PS: Decorations are icky.
 */
static BOX
smartputExtendBox (CompWindow *w,
		   BOX        tmp,
		   Region     r,
		   Bool       xFirst,
		   Bool       left,
		   Bool       right,
		   Bool       up,
		   Bool       down)
{
    short int counter = 0;
    Bool      touch = FALSE;
#define CHECKREC \
	XRectInRegion (r, tmp.x1 - w->input.left, tmp.y1 - w->input.top,\
		       tmp.x2 - tmp.x1 + w->input.left + w->input.right,\
		       tmp.y2 - tmp.y1 + w->input.top + w->input.bottom)\
	    == RectangleIn

    while (counter < 1)
    {
	if ((xFirst && counter == 0) || (!xFirst && counter == 1))
	{
	    if(left)
	    {
		while (CHECKREC) {
		    tmp.x1--;
		    touch = TRUE;
		}
		if (touch) tmp.x1++;
	    }
	    touch = FALSE;
	    if(right) {
		while (CHECKREC) {
		    tmp.x2++;
		    touch = TRUE;
		}
		if (touch) tmp.x2--;
	    }
	    touch = FALSE;
	    counter++;
	}

	if ((xFirst && counter == 1) || (!xFirst && counter == 0))
	{
	    if(down) {
		while (CHECKREC) {
		    tmp.y2++;
		    touch = TRUE;
		}
		if (touch) tmp.y2--;
	    }
	    touch = FALSE;

	    if(up) {
		while (CHECKREC) {
		    tmp.y1--;
		    touch = TRUE;
		}
		if (touch) tmp.y1++;
	    }
	    touch = FALSE;
	    counter++;
	}
    }
#undef CHECKREC
    return tmp;
}

/*
 * Createing boxes to test which empty rectangle is bigger.
 */

static void
smartputCreateBoxes (CompWindow* window,
		     BOX*        testBoxes)
{
    /* BOX to represent an infintesimal window to compute maximum empty region */
    XRectangle tmpRect;
    tmpRect.x = window->serverX - window->input.left;
    tmpRect.y = window->serverY - window->input.top;
    tmpRect.width  = window->serverWidth + window->input.right + window->input.left;
    tmpRect.height = window->serverHeight + window->input.top +
		     window->input.bottom;
    /* Upper left corner */
    testBoxes[0].x1 = tmpRect.x - 2;
    testBoxes[0].y1 = tmpRect.y - 2;
    testBoxes[0].x2 = tmpRect.x - 1;
    testBoxes[0].y2 = tmpRect.y - 1;

    /* Bottom left corner */
    testBoxes[1].x1 = tmpRect.x - 2;
    testBoxes[1].y1 = tmpRect.y + tmpRect.height + 1;
    testBoxes[1].x2 = tmpRect.x - 1;
    testBoxes[1].y2 = tmpRect.y + tmpRect.height + 2;

    /* Bottom Right corner */
    testBoxes[2].x1 = tmpRect.x + tmpRect.width + 1;
    testBoxes[2].y1 = tmpRect.y + tmpRect.height + 1;
    testBoxes[2].x2 = tmpRect.x + tmpRect.width + 2;
    testBoxes[2].y2 = tmpRect.y + tmpRect.height + 2;

    /* Upper Right corner */
    testBoxes[3].x1 = tmpRect.x + tmpRect.width + 1;
    testBoxes[3].y1 = tmpRect.y - 2;
    testBoxes[3].x2 = tmpRect.x + tmpRect.width + 2;
    testBoxes[3].y2 = tmpRect.y - 1;
}

static Bool
smartputValidBox (CompScreen* s,
		  BOX         box)
{
    if(box.x1 < 0 || box.y1 < 0 || box.x2 < 0 || box.y2 < 0) return FALSE;
    if(box.x1 > s->width || box.x2 > s->width || box.y1 > s->height || box.y2 > s->height) return FALSE;
    return TRUE;
}

/*
 *
 */
static BOX
smartputBox (CompWindow*	w,
	     Region             r,
	     BOX                testBox)
{
    BOX ansA, ansB;
    ansA = smartputExtendBox (w, testBox, r, TRUE, TRUE,TRUE,TRUE,TRUE);
    ansB = smartputExtendBox (w, testBox, r, FALSE, TRUE,TRUE,TRUE,TRUE);

    if (smartputBoxCompare (testBox, ansA) &&
	smartputBoxCompare (testBox, ansB))
	return testBox;
    if (smartputBoxCompare (ansA, ansB))
	return ansA;
    else
	return ansB;
}
/*
 * Box defined from rectangle
 */
static BOX
smartputBoxFromWindow (CompWindow *w)
{
    BOX windowBox;

    windowBox.x1 = w->serverX;
    windowBox.y1 = w->serverY;
    windowBox.x2 = w->serverX + w->serverWidth;
    windowBox.y2 = w->serverY + w->serverHeight;

    return windowBox;
}


/*
 * Box defined from rectangle
 */
static BOX
smartputBoxFromRect (XRectangle rect)
{
    BOX rectBox;

    rectBox.x1 = rect.x;
    rectBox.y1 = rect.y;
    rectBox.x2 = rect.x + rect.width;
    rectBox.y2 = rect.y + rect.height;

    return rectBox;
}

/*
 * Finding the best place to put the current window.
 */

static BOX
smartputFindRect (CompWindow *w,
		  Region      r)
{
    BOX windowBox,maxBox;

    XRectangle tmpRect;
    tmpRect.x = w->serverX;
    tmpRect.y = w->serverY;
    tmpRect.width  = w->serverWidth;
    tmpRect.height = w->serverHeight;

    windowBox = smartputBoxFromRect (tmpRect);

    int currentArea = tmpRect.width*tmpRect.height;

    CompScreen  *s = w->screen;
    CompDisplay *d = s->display;
    CompWindow  *window;

    int maxArea = -1;
    int area    = 0;

    for (window = s->windows; window; window = window->next)
    {
	BOX testBoxes[4];
	smartputCreateBoxes (window,testBoxes);
	int i;
	for(i = 0; i < 4; i++)
	{
	    BOX testBox = testBoxes[i];

	    BOX tmpMaxBox;
	    if(smartputValidBox (s,testBox))
	    {
		tmpMaxBox = smartputBox (w,r,testBox);
		area = (tmpMaxBox.x2 - tmpMaxBox.x1)*(tmpMaxBox.y2 - tmpMaxBox.y1);
		if(area >= maxArea)
		{
		    maxArea = area;
		    maxBox = tmpMaxBox;
		}
	    }
	}
    }

   /* Check if the user has choosen that windows also may shrink
    * And if so, windows may just shrink to a minimal size of 50x50
    */
    if(smartputGetAllowWindowsShrink (d) &&
       (maxBox.x2 - maxBox.x1) > smartputGetWindowMinwidth (d) &&
       (maxBox.y2 - maxBox.y1) > smartputGetWindowMinheight (d))
        return maxBox;

    if(maxArea >= currentArea)
        return maxBox;
    else
        return windowBox;
}

/*
 * Finding the best window to stack the current window on.
 */

static BOX
smartputFindStapleRect (CompWindow *w,
			Region      r)
{
    BOX windowBox, maxBox;
    CompWindow *window;
    int maxArea;

    windowBox = smartputBoxFromWindow (w);

    maxArea = -1;
    maxBox  = windowBox;
    for (window = w->screen->windows; window; window = window->next) {

        BOX current;

	current = smartputBoxFromWindow (window);

        if (window->invisible || window->hidden || window->minimized)
            continue;

        if (current.x1 == windowBox.x1 &&
	    current.x2 == windowBox.x2 &&
	    current.y1 == windowBox.y1 &&
	    current.y2 == windowBox.y2)
            continue;

        if (current.x1 == 0 &&
	    current.x2 == w->screen->width &&
	    current.y1 == 0 &&
	    current.y2 == w->screen->height)
            continue;

        int rXpos,  rYpos   = 0;
        int rWidth, rHeight = 0;
        if (current.x1 > windowBox.x1)
            rXpos = current.x1;
        else
            rXpos = windowBox.x1;

        if (current.x2 < windowBox.x2)
            rWidth = current.x2 - rXpos;
        else
            rWidth = windowBox.x2 - rXpos;

        if (current.y1 > windowBox.y1)
            rYpos = current.y1;
        else
            rYpos = windowBox.y1;

        if (current.y2 < windowBox.y2)
            rHeight = current.y2 - rYpos;
        else
            rHeight = windowBox.y2 - rYpos;

        int coverArea = 0;
        if (rWidth < 0 || rHeight < 0)
            coverArea = 0;
        else
            coverArea = rHeight * rWidth;

        if (coverArea > maxArea)
        {
            maxArea = coverArea;
            maxBox  = current;
        }
    }

    return maxBox;

}

/*
 * Calls out to compute the resize.
 */
static unsigned int
smartputComputeResize (CompWindow     *w,
		       XWindowChanges *xwc)
{
    CompOutput   *output;
    CompDisplay  *d;
    Region       region;
    unsigned int mask = 0;
    BOX          box;

    output = &w->screen->outputDev[outputDeviceForWindow (w)];
    region = smartputEmptyRegion (w, &output->region);
    d      = w->screen->display;

    if (!region)
	return mask;

    box = smartputFindRect (w, region);

    /* Find out if the box which is given back by smartputFindRect
     * has other dimensions than the current window
     * If it has not, the window will not move and if the users
     * set it in the options we will try to stack it onto an
     * underlying window
     */
     if (box.x1 == w->serverX &&
        box.y1 == w->serverY &&
        (box.x2 - box.x1) == w->serverWidth &&
        (box.y2 - box.y1) == w->serverHeight &&
        smartputGetAllowWindowsStack (d))
    {
        box = smartputFindStapleRect(w, region);
    }
    else
    {
        /* The margin stuff which is set up in the options */
        int margin = smartputGetWindowMargin (d);
        box.x1 = box.x1 + margin;
        box.x2 = box.x2 - margin;
        box.y1 = box.y1 + margin;
        box.y2 = box.y2 - margin;
    }

    XDestroyRegion (region);

    if (box.x1 != w->serverX)
	mask |= CWX;

    if (box.y1 != w->serverY)
	mask |= CWY;

    if ((box.x2 - box.x1) != w->serverWidth)
	mask |= CWWidth;

    if ((box.y2 - box.y1) != w->serverHeight)
	mask |= CWHeight;

    xwc->x = box.x1;
    xwc->y = box.y1;
    xwc->width = box.x2 - box.x1;
    xwc->height = box.y2 - box.y1;

    return mask;
}

/*
 * Testing if two windows are on the same viewport
 */

static Bool
smartputSameViewport (CompWindow* w1,
		      CompWindow* w2)
{
    int x1,x2,y1,y2;

    defaultViewportForWindow (w1,&x1,&y1);
    defaultViewportForWindow (w2,&x2,&y2);

    return ((x1 == x2) && (y1 == y2));
}

/*
 * Update undo information.
 */
static void
smartputUpdateUndoInfo (CompScreen *s,
			CompWindow *w,
			XWindowChanges *xwc,
			Bool       reset)
{
    SMARTPUT_SCREEN (s);
    if(reset)
    {
	sps->undoInfo.window      = None;
	sps->undoInfo.savedX      = 0;
	sps->undoInfo.savedY      = 0;
	sps->undoInfo.savedHeight = 0;
	sps->undoInfo.savedWidth  = 0;
	sps->undoInfo.newX        = 0;
	sps->undoInfo.newY        = 0;
	sps->undoInfo.newHeight   = 0;
	sps->undoInfo.newWidth    = 0;
	sps->undoInfo.state       = 0;
    }
    else
    {
	sps->undoInfo.window      = w->id;
	sps->undoInfo.savedX      = w->serverX;
	sps->undoInfo.savedY      = w->serverY;
	sps->undoInfo.savedWidth  = w->serverWidth;
	sps->undoInfo.savedHeight = w->serverHeight;
	sps->undoInfo.newX        = xwc->x;
	sps->undoInfo.newY        = xwc->y;
	sps->undoInfo.newHeight   = xwc->height;
	sps->undoInfo.newWidth    = xwc->width;
	sps->undoInfo.state       = w->state;
    }
}

/*
 * Compute size based on undo information.
 */

static unsigned int
smartputUndoResize (CompScreen     *s,
		    XWindowChanges *xwc)
{
    unsigned int mask = 0;

    SMARTPUT_SCREEN (s);

    mask |= CWWidth | CWHeight | CWX | CWY;
    xwc->x      = sps->undoInfo.savedX;
    xwc->y      = sps->undoInfo.savedY;
    xwc->width  = sps->undoInfo.savedWidth;
    xwc->height = sps->undoInfo.savedHeight;

    return mask;
}

static Bool
smartputInitiate (CompWindow      *w,
		  CompAction      *action,
		  CompActionState state,
		  CompOption	 *option,
		  int             nOption,
		  Bool            undo)
{
    CompDisplay *d;
    if (w)
    {
	CompScreen* s = w->screen;

	SMARTPUT_SCREEN (s);

	if(otherScreenGrabExist (s, "smartput", 0))
	    return FALSE;
	/*
	 * Grab the screen
	 */
	if(!sps->grabIndex)
	    sps->grabIndex = pushScreenGrab (s, s->invisibleCursor, "smartput");
	if(!sps->grabIndex)
	    return FALSE;

	int            width, height;
	unsigned int   mask;
	XWindowChanges *xwc;

	d = s->display;
	SMARTPUT_WINDOW (w);

	if(spw->xwc) free(spw->xwc);
	xwc = malloc(sizeof(XWindowChanges));

	if(undo)
	{
	    if(!(w->id == sps->undoInfo.window) || sps->undoInfo.window == None) return FALSE;
	    mask = smartputUndoResize (s,xwc);
	}
	else if(w->id == sps->undoInfo.window && !(sps->undoInfo.window == None) &&
		sps->undoInfo.newX == w->serverX &&
		sps->undoInfo.newY == w->serverY &&
		sps->undoInfo.newWidth == w->serverWidth &&
		sps->undoInfo.newHeight == w->serverHeight &&
		smartputGetUseTriggerkeyForundo (d))
	{
	    mask = smartputUndoResize (s,xwc);
	    undo = TRUE;
	}
	else
	{
	    mask = smartputComputeResize (w, xwc);
	}

	if (mask)
	{
	    if (constrainNewWindowSize (w, xwc->width, xwc->height,
					&width, &height))
	    {
		mask |= CWWidth | CWHeight;
		xwc->width  = width;
		xwc->height = height;
	    }
	    spw->lastX = w->serverX;
	    spw->lastY = w->serverY;

	    spw->targetX    = xwc->x;
	    spw->targetY    = xwc->y;
	    spw->xwc        = xwc;
	    spw->mask       = mask;
	    sps->lastWindow = w->id;

	    smartputUpdateUndoInfo (s, w, xwc, undo);

	    spw->animation = TRUE;
	    sps->animation = TRUE;

	    addWindowDamage (w);
	}
    }

    return TRUE;

}

/*
 * Tries to undo Smart Put last action
 */
static Bool
smartputUndo (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    if(!xid)
	xid = d->activeWindow;

    w   = findWindowAtDisplay (d, xid);

    if (w)
    {
	return smartputInitiate (w, action, state,
				 option, nOption,TRUE);

    }
    return FALSE;
}

/*
 * Initially triggered keybinding.
 * Fetch the window, fetch the resize, constrain it.
 *
 */
static Bool
smartputTrigger (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    if(!xid)
	xid = d->activeWindow;

    w   = findWindowAtDisplay (d, xid);

    if (w)
    {
	if (w->invisible || w->hidden || w->minimized)
	    return FALSE;

	if (w->wmType & CompWindowTypeDesktopMask)
	    return FALSE;

	if (w->wmType & CompWindowTypeDockMask)
	    return FALSE;
	return smartputInitiate (w, action, state,
				 option, nOption,FALSE);

    }
    return FALSE;
}


/*
 * Buggy
 */
static Bool
smartputAllTrigger (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
    Window     xid;
    CompWindow *w;
    CompScreen *s;
    int grabIndex = 0;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    if (w)
    {
	s = w->screen;

	if(otherScreenGrabExist (s, "smartput", 0))
	    return FALSE;

	/*
	 * Grab the screen
	 */

	grabIndex = pushScreenGrab (s, s->invisibleCursor, "smartput");

	if(!grabIndex)
	    return FALSE;

	CompWindow *window;
	for (window = s->windows; window; window = window->next)
	{
	    if(!smartputSameViewport (window,w) )
	       continue;
	    int            width, height;
	    unsigned int   mask;
	    XWindowChanges xwc;

	    mask = smartputComputeResize (window, &xwc);
	    if (mask)
	    {
		if (constrainNewWindowSize (window, xwc.width, xwc.height,
					    &width, &height))
		{
		    mask |= CWWidth | CWHeight;
		    xwc.width  = width;
		    xwc.height = height;
		}

		if (window->mapNum && (mask & (CWWidth | CWHeight)))
		    sendSyncRequest (window);

		configureXWindow (window, mask, &xwc);
	    }
	}
    }
    if(grabIndex)
	removeScreenGrab (s,grabIndex, NULL);
    return TRUE;
}

/*
 * Calculate the velocity for the moving window
 */
static int
adjustSmartputVelocity (CompWindow *w)
{
    float dx, dy, adjust, amount;
    float x1, y1;

    SMARTPUT_WINDOW (w);

    x1 = spw->targetX;
    y1 = spw->targetY;

    dx = x1 - (w->attrib.x + spw->tx);
    dy = y1 - (w->attrib.y + spw->ty);

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    spw->xVelocity = (amount * spw->xVelocity + adjust) / (amount + 1.0f);

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    spw->yVelocity = (amount * spw->yVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (spw->xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (spw->yVelocity) < 0.2f)
    {
	/* animation done */
	spw->xVelocity = spw->yVelocity = 0.0f;

	spw->tx = x1 - w->attrib.x;
	spw->ty = y1 - w->attrib.y;
	return 0;
    }
    return 1;
}

/*
 * Setup for paint screen
 */
static void
smartputPreparePaintScreen (CompScreen *s,
			    int        msSinceLastPaint)
{
    SMARTPUT_SCREEN (s);

    if (sps->animation && sps->grabIndex)
    {
	CompWindow *w;
	int        stesps;
	float      amount, chunk;

	amount = msSinceLastPaint * 0.025f * 2.5;
	stesps = amount / (0.5f * 0.5);
	if (!stesps)
	    stesps = 1;
	chunk = amount / (float)stesps;

	while (stesps--)
	{
	    Window endAnimationWindow = None;

	    sps->animation = 0;
	    for (w = s->windows; w; w = w->next)
	    {
		SMARTPUT_WINDOW (w);

		if (spw->animation)
		{
		    spw->animation = adjustSmartputVelocity (w);
		    sps->animation |= spw->animation;

		    spw->tx += spw->xVelocity * chunk;
		    spw->ty += spw->yVelocity * chunk;

		    if (!spw->animation)
		    {
			/* animation done */
			moveWindow (w, spw->targetX - w->attrib.x,
				    spw->targetY - w->attrib.y, TRUE, TRUE);
			syncWindowPosition (w);
			updateWindowAttributes (w, CompStackingUpdateModeNone);
			endAnimationWindow = w->id;
			spw->tx = spw->ty = 0;
		    }
		}
	    }
	    if (!sps->animation)
	    {
		/* unfocus moved window if enabled */
		if (endAnimationWindow)
		    sendWindowActivationRequest (s, endAnimationWindow);
		break;
	    }
	}
    }

    UNWRAP (sps, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (sps, s, preparePaintScreen, smartputPreparePaintScreen);
}


/*
 * After painting clean up
 */
static void
smartputDonePaintScreen (CompScreen *s)
{
    CompWindow *w = NULL;

    SMARTPUT_SCREEN (s);
    if(sps->lastWindow != None)
    {
        w = findWindowAtScreen (s,sps->lastWindow);
    }

    if (sps->animation && sps->grabIndex)
	damageScreen (s);
    else
    {
	if(w)
	{
	    SMARTPUT_WINDOW (w);
	    if(spw->mask && spw->xwc)
	    {
		if (w->mapNum && (spw->mask & (CWWidth | CWHeight)))
		{
		    sendSyncRequest (w);
		    configureXWindow (w, spw->mask, spw->xwc);
		}
		spw->mask = 0;
		free(spw->xwc);
		spw->xwc  = NULL;
	    }
	}
	if (sps->grabIndex)
	{
	    /* release the screen grab */
	    removeScreenGrab (s, sps->grabIndex, NULL);
	    sps->grabIndex = 0;
	}
    }

    UNWRAP (sps, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (sps, s, donePaintScreen, smartputDonePaintScreen);
}

static Bool
smartputPaintOutput (CompScreen              *s,
		     const ScreenPaintAttrib *sAttrib,
		     const CompTransform     *transform,
		     Region                  region,
		     CompOutput              *output,
		     unsigned int            mask)
{
    Bool status;

    SMARTPUT_SCREEN (s);

    if (sps->animation)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (sps, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (sps, s, paintOutput, smartputPaintOutput);

    return status;
}

static void
smartputDrawRect (CompWindow *w)
{
    SMARTPUT_WINDOW (w);
    glPushMatrix ();
    glBegin (GL_LINE_LOOP);
    glVertex2f(spw->targetX - w->input.left ,
	       spw->targetY - w->input.top);
    glVertex2f(spw->targetX + w->input.right ,
	       spw->targetY + spw->xwc->height
	       + w->input.bottom);
    glVertex2f(spw->targetX + spw->xwc->width
	       + w->input.right ,
	       spw->targetY + spw->xwc->height
	       + w->input.bottom);
    glVertex2f(spw->targetX + spw->xwc->width
	       - w->input.left ,
	       spw->targetY - w->input.top );
    glEnd ();
    glPopMatrix ();
}


static Bool
smartputPaintWindow (CompWindow              *w,
		     const WindowPaintAttrib *attrib,
		     const CompTransform     *transform,
		     Region                  region,
		     unsigned int            mask)
{
    CompScreen *s = w->screen;
    Bool status;

    SMARTPUT_SCREEN (s);
    SMARTPUT_WINDOW (w);

    if (spw->animation)
    {
	smartputDrawRect (w);

	CompTransform wTransform = *transform;

	matrixTranslate (&wTransform, spw->tx, spw->ty, 0.0f);

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	UNWRAP (sps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, &wTransform, region, mask);
	WRAP (sps, s, paintWindow, smartputPaintWindow);
    }
    else
    {
	UNWRAP (sps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (sps, s, paintWindow, smartputPaintWindow);
    }

    return status;
}

static void
smartputHandleEvent (CompDisplay *d,
		    XEvent      *event)
{
    SMARTPUT_DISPLAY (d);

    switch (event->type)
    {
	/* handle client events */
    case ClientMessage:
	/* accept the custom atom for putting windows */
	if (event->xclient.message_type == spd->compizSmartputWindowAtom)
	{
	    CompWindow *w;

	    w = findWindowAtDisplay (d, event->xclient.window);
	    if (w)
	    {
		/*
		 * get the values from the xclientmessage event and populate
		 * the options for put initiate
		 *
		 * the format is 32
		 * and the data is
		 * l[0] = x position - unused (for future PutExact)
		 * l[1] = y position - unused (for future PutExact)
		 * l[2] = face number
		 * l[3] = put type, int value from enum
		 * l[4] = Xinerama head number
		 */
		CompOption opt[5];

		opt[0].type    = CompOptionTypeInt;
		opt[0].name    = "window";
		opt[0].value.i = event->xclient.window;

		opt[1].type    = CompOptionTypeInt;
		opt[1].name    = "x";
		opt[1].value.i = event->xclient.data.l[0];

		opt[2].type    = CompOptionTypeInt;
		opt[2].name    = "y";
		opt[2].value.i = event->xclient.data.l[1];

		opt[3].type    = CompOptionTypeInt;
		opt[3].name    = "face";
		opt[3].value.i = event->xclient.data.l[2];

		opt[4].type    = CompOptionTypeInt;
		opt[4].name    = "head";
		opt[4].value.i = event->xclient.data.l[4];

		smartputTrigger (w->screen->display, NULL, 0, opt, 5);
	    }
	}
	break;
    default:
	break;
    }

    UNWRAP (spd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (spd, d, handleEvent, smartputHandleEvent);
}

/* Configuration, initialization, boring stuff. --------------------- */
static Bool
smartputInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    SmartputDisplay *spd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    spd = malloc (sizeof (SmartputDisplay));
    if (!spd)
	return FALSE;

    spd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (spd->screenPrivateIndex < 0)
    {
	free (spd);
	return FALSE;
    }

    /* custom atom for client events */
    spd->compizSmartputWindowAtom = XInternAtom(d->display,
						"_COMPIZ_SMARTPUT_WINDOW", 0);

    spd->lastWindow = None;

    smartputSetTriggerButtonInitiate(d, smartputTrigger);
    smartputSetTriggerAllButtonInitiate (d, smartputAllTrigger);
    smartputSetTriggerKeyInitiate (d, smartputTrigger);
    smartputSetUndoKeyInitiate (d, smartputUndo);

    WRAP (spd, d, handleEvent, smartputHandleEvent);
    d->base.privates[SmartputDisplayPrivateIndex].ptr = spd;

    return TRUE;
}

static void
smartputFiniDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    SMARTPUT_DISPLAY (d);

    freeScreenPrivateIndex (d, spd->screenPrivateIndex);
    UNWRAP (spd, d, handleEvent);

    free (spd);
}

static Bool
smartputInitScreen (CompPlugin *p,
		    CompScreen *s)
{
    SmartputScreen *sps;

    SMARTPUT_DISPLAY (s->display);

    sps = malloc (sizeof (SmartputScreen));
    if (!sps)
	return FALSE;

    sps->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (sps->windowPrivateIndex < 0)
    {
	free (sps);
	return FALSE;
    }

    /* initialize variables
     * bad stuff happens if we don't do this
     */
    sps->animation = FALSE;
    sps->grabIndex = 0;
    sps->lastWindow = None;

    /* wrap the overloaded functions */
    WRAP (sps, s, preparePaintScreen, smartputPreparePaintScreen);
    WRAP (sps, s, donePaintScreen, smartputDonePaintScreen);
    WRAP (sps, s, paintOutput, smartputPaintOutput);
    WRAP (sps, s, paintWindow, smartputPaintWindow);

    s->base.privates[spd->screenPrivateIndex].ptr = sps;
    return TRUE;
}

static void
smartputFiniScreen (CompPlugin *p,
		    CompScreen *s)
{
    SMARTPUT_SCREEN (s);

    freeWindowPrivateIndex (s, sps->windowPrivateIndex);

    UNWRAP (sps, s, preparePaintScreen);
    UNWRAP (sps, s, donePaintScreen);
    UNWRAP (sps, s, paintOutput);
    UNWRAP (sps, s, paintWindow);

    free (sps);
}


static Bool
smartputInitWindow (CompPlugin *p,
		    CompWindow *w)
{
    SmartputWindow *spw;

    SMARTPUT_SCREEN (w->screen);

    spw = malloc (sizeof (SmartputWindow));
    if (!spw)
	return FALSE;

    /* initialize variables
     * I don't need to repeat it
     */
    spw->tx = spw->ty = spw->xVelocity = spw->yVelocity = 0.0f;
    spw->lastX = w->serverX;
    spw->lastY = w->serverY;
    spw->animation = FALSE;
    spw->xwc = 0;
    spw->mask = 0;

    w->base.privates[sps->windowPrivateIndex].ptr = spw;

    return TRUE;
}

static void
smartputFiniWindow (CompPlugin *p,
		   CompWindow *w)
{
    SMARTPUT_WINDOW (w);
    if(spw->xwc) free(spw->xwc);
    free (spw);
}

static CompBool
smartputInitObject (CompPlugin *p,
		    CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) smartputInitDisplay,
	(InitPluginObjectProc) smartputInitScreen,
	(InitPluginObjectProc) smartputInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
smartputFiniObject (CompPlugin *p,
		    CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) smartputFiniDisplay,
	(FiniPluginObjectProc) smartputFiniScreen,
	(FiniPluginObjectProc) smartputFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
smartputInit (CompPlugin *p)
{
    SmartputDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (SmartputDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
smartputFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (SmartputDisplayPrivateIndex);
}



CompPluginVTable smartputVTable = {
    "smartput",
    0,
    smartputInit,
    smartputFini,
    smartputInitObject,
    smartputFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &smartputVTable;
}
