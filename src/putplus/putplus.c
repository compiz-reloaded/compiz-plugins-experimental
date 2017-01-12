/*
 * Compiz Application Put Plus plugin
 *
 * putplus.c
 *
 * Copyright : (C) 2008 by Eduardo Gurgel
 * E-mail    : edgurgel@gmail.com
 *
 * Copyright : (C) 2008 by Marco Diego Aurelio Mesquita
 * E-mail    : marcodiegomesquita@gmail.com
 *
 * Based on put.c:
 * Copyright : (C) 2006 Darryll Truchan
 * E-mail    : <moppsy@comcast.net>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Authors:
 * Darryll Truchan              <moppsy@comcast.net>
 * Eduardo Gurgel Pinho         <edgurgel@gmail.com>
 * Marco Diego Aurélio Mesquita <marcodiegomesquita@gmail.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <X11/Xatom.h>

#include <compiz-core.h>
#include "putplus_options.h"

#define GET_PUTPLUS_DISPLAY(d) \
    ((PutplusDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define PUTPLUS_DISPLAY(d) \
    PutplusDisplay *pd = GET_PUTPLUS_DISPLAY (d)

#define GET_PUTPLUS_SCREEN(s, pd) \
    ((PutplusScreen *) (s)->base.privates[(pd)->screenPrivateIndex].ptr)
#define PUTPLUS_SCREEN(s) \
    PutplusScreen *ps = GET_PUTPLUS_SCREEN (s, GET_PUTPLUS_DISPLAY (s->display))

#define GET_PUTPLUS_WINDOW(w, ps) \
    ((PutplusWindow *) (w)->base.privates[(ps)->windowPrivateIndex].ptr)
#define PUTPLUS_WINDOW(w) \
    PutplusWindow *pw = GET_PUTPLUS_WINDOW (w, \
		    GET_PUTPLUS_SCREEN (w->screen, \
		    GET_PUTPLUS_DISPLAY (w->screen->display)))
#define PUTPLUS_ONLY_EMPTY(type) (type >= PutplusEmptyBottomLeft && type <= PutplusEmptyTopRight)
static int displayPrivateIndex;

typedef enum
{
    PutplusUnknown = 0,
    PutplusBottomLeft = 1,
    PutplusBottom = 2,
    PutplusBottomRight = 3,
    PutplusLeft = 4,
    PutplusCenter = 5,
    PutplusRight = 6,
    PutplusTopLeft = 7,
    PutplusTop = 8,
    PutplusTopRight = 9,
    PutplusRestore = 10,
    PutplusViewport = 11,
    PutplusViewportLeft = 12,
    PutplusViewportRight = 13,
    PutplusAbsolute = 14,
    PutplusPointer = 15,
    PutplusViewportUp = 16,
    PutplusViewportDown = 17,
    PutplusRelative = 18,
    PutplusEmptyBottomLeft = 19,
    PutplusEmptyBottom = 20,
    PutplusEmptyBottomRight = 21,
    PutplusEmptyLeft = 22,
    PutplusEmptyCenter = 23,
    PutplusEmptyRight = 24,
    PutplusEmptyTopLeft = 25,
    PutplusEmptyTop = 26,
    PutplusEmptyTopRight = 27
} PutplusType;

typedef struct _PutplusDisplay
{
    int screenPrivateIndex;

    HandleEventProc handleEvent; /* handle event function pointer */

    Window  lastWindow;
    PutplusType lastType;

    Atom compizPutplusWindowAtom;    /* client event atom         */
} PutplusDisplay;

typedef struct _PutplusScreen
{
    int windowPrivateIndex;

    PreparePaintScreenProc preparePaintScreen;	/* function pointer         */
    DonePaintScreenProc    donePaintScreen;	/* function pointer         */
    PaintOutputProc        paintOutput;	        /* function pointer         */
    PaintWindowProc        paintWindow;	        /* function pointer         */

    int        moreAdjust;			/* animation flag           */
    int        grabIndex;			/* screen grab index        */
} PutplusScreen;

typedef struct _PutplusWindow
{
    GLfloat xVelocity, yVelocity;	/* animation velocity       */
    GLfloat tx, ty;			/* animation translation    */

    int lastX, lastY;			/* starting position        */
    int targetX, targetY;               /* target of the animation  */

    Bool adjust;			/* animation flag           */
} PutplusWindow;


/* Generates a region containing free space (here the
 * active window counts as free space). The region argument
 * is the start-region (ie: the output dev).
 * Logic borrowed from opacify (courtesy of myself).
 */
static Region
putplusEmptyRegion (CompWindow *window,
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
putplusBoxCompare (BOX a,
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
putplusExtendBox (CompWindow *w,
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
		if(left) {
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

/* Create a box for resizing in the given region
 * Also shrinks the window box in case of minor overlaps.
 * FIXME: should be somewhat cleaner.
 */
static BOX
putplusFindRect (CompWindow *w,
		 Region     r,
		 Bool       left,
		 Bool       right,
		 Bool       up,
		 Bool       down)
{
    BOX windowBox, ansA, ansB, orig;

    windowBox.x1 = w->serverX;
    windowBox.x2 = w->serverX + w->serverWidth;
    windowBox.y1 = w->serverY;
    windowBox.y2 = w->serverY + w->serverHeight;

    orig = windowBox;

    if (windowBox.x2 - windowBox.x1 > 80)
    {
			if(left) windowBox.x1 += 40;
			if(right)	windowBox.x2 -= 40;
    }
    if (windowBox.y2 - windowBox.y1 > 80)
    {
			if(up) windowBox.y1 += 40;
			if(down) windowBox.y2 -= 40;
    }

    ansA = putplusExtendBox (w, windowBox, r, TRUE, left, right, up, down);
    ansB = putplusExtendBox (w, windowBox, r, FALSE, left, right, up, down);

    if (putplusBoxCompare (orig, ansA) &&
	putplusBoxCompare (orig, ansB))
	return orig;
    if (putplusBoxCompare (ansA, ansB))
	return ansA;
    else
	return ansB;

}

/* Calls out to compute the resize */
static unsigned int
putplusComputeResize(CompWindow     *w,
		     XWindowChanges *xwc,
		     Bool           left,
		     Bool           right,
		     Bool           up,
		     Bool           down)
{
    CompOutput   *output;
    Region       region;
    unsigned int mask = 0;
    BOX          box;

    output = &w->screen->outputDev[outputDeviceForWindow (w)];
    region = putplusEmptyRegion (w, &output->region);
    if (!region)
	return mask;

    box = putplusFindRect (w, region, left, right, up, down);
    XDestroyRegion (region);

    if (box.x1 != w->serverX)
	mask |= CWX;

    if (box.y1 != w->serverY)
	mask |= CWY;

    if ((box.x2 - box.x1) != w->serverWidth)
	mask |= CWWidth;

    if ((box.y2 - box.y1) != w->height)
	mask |= CWHeight;

    xwc->x = box.x1;
    xwc->y = box.y1;
    xwc->width = box.x2 - box.x1;
    xwc->height = box.y2 - box.y1;

    return mask;
}


/*
 * End of Maximumize functions
 */


/*
 * calculate the velocity for the moving window
 */
static int
adjustPutplusVelocity (CompWindow *w)
{
    float dx, dy, adjust, amount;
    float x1, y1;

    PUTPLUS_WINDOW (w);

    x1 = pw->targetX;
    y1 = pw->targetY;

    dx = x1 - (w->attrib.x + pw->tx);
    dy = y1 - (w->attrib.y + pw->ty);

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    pw->xVelocity = (amount * pw->xVelocity + adjust) / (amount + 1.0f);

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    pw->yVelocity = (amount * pw->yVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (pw->xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (pw->yVelocity) < 0.2f)
    {
	/* animation done */
	pw->xVelocity = pw->yVelocity = 0.0f;

	pw->tx = x1 - w->attrib.x;
	pw->ty = y1 - w->attrib.y;
	return 0;
    }
    return 1;
}

/*
 * setup for paint screen
 */
static void
putplusPreparePaintScreen (CompScreen *s,
			   int        msSinceLastPaint)
{
    PUTPLUS_SCREEN (s);

    if (ps->moreAdjust && ps->grabIndex)
    {
	CompWindow *w;
	int        steps;
	float      amount, chunk;

	amount = msSinceLastPaint * 0.025f * putplusGetSpeed (s);
	steps = amount / (0.5f * putplusGetTimestep (s));
	if (!steps)
	    steps = 1;
	chunk = amount / (float)steps;

	while (steps--)
	{
	    Window endAnimationWindow = None;

	    ps->moreAdjust = 0;
	    for (w = s->windows; w; w = w->next)
	    {
		PUTPLUS_WINDOW (w);

		if (pw->adjust)
		{
		    pw->adjust = adjustPutplusVelocity (w);
		    ps->moreAdjust |= pw->adjust;

		    pw->tx += pw->xVelocity * chunk;
		    pw->ty += pw->yVelocity * chunk;

		    if (!pw->adjust)
		    {
			/* animation done */
			moveWindow (w, pw->targetX - w->attrib.x,
				    pw->targetY - w->attrib.y, TRUE, TRUE);
			syncWindowPosition (w);
			updateWindowAttributes (w, CompStackingUpdateModeNone);
			endAnimationWindow = w->id;
			pw->tx = pw->ty = 0;
		    }
		}
	    }
	    if (!ps->moreAdjust)
	    {
		/* unfocus moved window if enabled */
		if (putplusGetUnfocusWindow (s))
		    focusDefaultWindow (s);
		else if (endAnimationWindow)
		    sendWindowActivationRequest (s, endAnimationWindow);
		break;
	    }
	}
    }

    UNWRAP (ps, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ps, s, preparePaintScreen, putplusPreparePaintScreen);
}

/*
 * after painting clean up
 */
static void
putplusDonePaintScreen (CompScreen *s)
{
    PUTPLUS_SCREEN (s);

    if (ps->moreAdjust && ps->grabIndex)
	damageScreen (s);
    else
    {
	if (ps->grabIndex)
	{
	    /* release the screen grab */
	    removeScreenGrab (s, ps->grabIndex, NULL);
	    ps->grabIndex = 0;
	}
    }

    UNWRAP (ps, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ps, s, donePaintScreen, putplusDonePaintScreen);
}

static Bool
putplusPaintOutput (CompScreen              *s,
		    const ScreenPaintAttrib *sAttrib,
		    const CompTransform     *transform,
		    Region                  region,
		    CompOutput              *output,
		    unsigned int            mask)
{
    Bool status;

    PUTPLUS_SCREEN (s);

    if (ps->moreAdjust)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (ps, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (ps, s, paintOutput, putplusPaintOutput);

    return status;
}

static Bool
putplusPaintWindow (CompWindow              *w,
		    const WindowPaintAttrib *attrib,
		    const CompTransform     *transform,
		    Region                  region,
		    unsigned int            mask)
{
    CompScreen *s = w->screen;
    Bool status;

    PUTPLUS_SCREEN (s);
    PUTPLUS_WINDOW (w);

    if (pw->adjust)
    {
	CompTransform wTransform = *transform;

	matrixTranslate (&wTransform, pw->tx, pw->ty, 0.0f);

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	UNWRAP (ps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, &wTransform, region, mask);
	WRAP (ps, s, paintWindow, putplusPaintWindow);
    }
    else
    {
	UNWRAP (ps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ps, s, paintWindow, putplusPaintWindow);
    }

    return status;
}

/*
 * initiate action callback
 */
static Bool
putplusInitiateCommon (CompDisplay     *d,
		       CompAction      *action,
		       CompActionState state,
		       CompOption      *option,
		       int             nOption,
		       PutplusType     type,
		       Bool            left,
		       Bool            right,
		       Bool            up,
		       Bool            down
		       )
{
    CompWindow *w;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    if (!xid)
	xid = d->activeWindow;

    w = findWindowAtDisplay (d, xid);
    if (w)
    {
	CompScreen *s = w->screen;

	PUTPLUS_SCREEN (s);

	/* If Putplus will act only on empty spaces we need to wait any animation*/
	if (PUTPLUS_ONLY_EMPTY(type))
	{
	    /* this will keep put from working while something
	       else has a screen grab */
	    if (otherScreenGrabExist (s, 0))
		return FALSE;

	    /* we are ok, so grab the screen */
	    if(!ps->grabIndex)
		ps->grabIndex = pushScreenGrab (s, s->invisibleCursor, "putplus");
	}
	else {

	    if (otherScreenGrabExist (s,"putplus", 0))
		return FALSE;

	    /* we are ok, so grab the screen */
	    if(!ps->grabIndex)
		ps->grabIndex = pushScreenGrab (s, s->invisibleCursor, "putplus");
	}

	if (ps->grabIndex)
	{
	    int        px, py, x, y, dx, dy;
	    int        head, width, height, hx, hy;
	    XRectangle workArea;

	    PUTPLUS_DISPLAY (d);
	    PUTPLUS_WINDOW (w);

	    px = getIntOptionNamed (option, nOption, "x", 0);
	    py = getIntOptionNamed (option, nOption, "y", 0);

	    /* we don't want to do anything with override redirect windows */
	    if (w->attrib.override_redirect)
		return FALSE;

	    /* we don't want to be moving the desktop, docks,
	       or fullscreen windows */
	    if (w->type & CompWindowTypeDesktopMask ||
		w->type & CompWindowTypeDockMask ||
		w->type & CompWindowTypeFullscreenMask)
	    {
		return FALSE;
	    }

	    /* get the Xinerama head from the options list */
	    head = getIntOptionNamed(option, nOption, "head", -1);
	    /* no head in options list so we use the current head */
	    if (head == -1)
	    {
		/* no head given, so use the current head if this wasn't
		   a double tap */
		if (pd->lastType != type || pd->lastWindow != w->id)
		{
		    if (pw->adjust)
		    {
			/* outputDeviceForWindow uses the server geometry,
			   so specialcase a running animation, which didn't
			   apply the server geometry yet */
			head = outputDeviceForGeometry (s,
							w->attrib.x + pw->tx,
							w->attrib.y + pw->ty,
							w->attrib.width,
							w->attrib.height,
							w->attrib.border_width);
		    }
		    else
			head = outputDeviceForWindow (w);
		}
	    }
	    else
	    {
		/* make sure the head number is not out of bounds */
		head = MIN (head, s->nOutputDev - 1);
	    }

	    if (head == -1)
	    {
		/* user double-tapped the key, so use the screen work area */
		workArea = s->workArea;
		/* set the type to unknown to have a toggle-type behavior
		   between 'use head's work area' and 'use screen work area' */
		pd->lastType = PutplusUnknown;
	    }
	    else
	    {
		/* single tap or head provided via options list,
		   use the head's work area */
		getWorkareaForOutput (s, head, &workArea);
		pd->lastType = type;
	    }

	    /* Checking if put will act only on empty spaces */

//	    if(type >= PutplusEmptyBottomLeft && type <= PutplusEmptyTopRight)
	    if(PUTPLUS_ONLY_EMPTY(type))
	    {
		unsigned int   mask;
		XWindowChanges xwc;

		memset(&xwc, 0, sizeof (XWindowChanges));
		mask = putplusComputeResize (w, &xwc, left,right,up,down);
		if (mask)
		{
		    int width_, height_;
		    if (constrainNewWindowSize (w, xwc.width, xwc.height,
						&width_, &height_))
		    {
			mask |= CWWidth | CWHeight;
			xwc.width  = width_;
			xwc.height = height_;
		    }
		}
		width  = xwc.width;
		height = xwc.height;
		hx     = xwc.x;
		hy     = xwc.y;
	    }
	    else
	    {
		width  = workArea.width;
		height = workArea.height;
		hx     = workArea.x;
		hy     = workArea.y;
	    }
	    pd->lastWindow = w->id;

	    /* the windows location */
	    x = w->attrib.x + pw->tx;
	    y = w->attrib.y + pw->ty;

	    /*
	     * handle the put types
	     */
	    switch (type)
	    {
	    case PutplusEmptyCenter:
	    case PutplusCenter:
		/* center of the screen */
		dx = (width / 2) - (w->width / 2) - (x - hx);
		dy = (height / 2) - (w->height / 2) - (y - hy);
		break;
	    case PutplusLeft:
		/* center of the left edge */
		dx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		dy = (height / 2) - (w->height / 2) - (y - hy);
		break;
	    case PutplusEmptyLeft:
		/* center of the left edge */
		hx = hx - w->input.left;
		dx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		dy = (height / 2) - (w->height / 2) - (y - hy);
		break;
	    case PutplusTopLeft:
		/* top left corner */
		dx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		dy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		break;
	    case PutplusEmptyTopLeft:
		/* top left corner */
		hy = hy - w->input.top;
		hx = hx - w->input.left;
		dx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		dy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		break;
	    case PutplusTop:
		/* center of top edge */
		dx = (width / 2) - (w->width / 2) - (x - hx);
		dy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		break;
	    case PutplusEmptyTop:
		/* center of top edge */
		hy = hy - w->input.top;
		dx = (width / 2) - (w->width / 2) - (x - hx);
		dy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		break;
	    case PutplusTopRight:
		/* top right corner */
		dx = width - w->width - (x - hx) -
		     w->input.right - putplusGetPadRight (s);
		dy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		break;
	    case PutplusEmptyTopRight:
		/* top right corner */
		hy = hy - w->input.top;
		hx = hx + w->input.right;
		dx = width - w->width - (x - hx) -
		     w->input.right - putplusGetPadRight (s);
		dy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		break;
	    case PutplusRight:
		/* center of right edge */
		dx = width - w->width - (x - hx) -
		     w->input.right - putplusGetPadRight (s);
		dy = (height / 2) - (w->height / 2) - (y - hy);
		break;
	    case PutplusEmptyRight:
		/* center of right edge */
		hx = hx + w->input.right;
		dx = width - w->width - (x - hx) -
		     w->input.right - putplusGetPadRight (s);
		dy = (height / 2) - (w->height / 2) - (y - hy);
		break;
	    case PutplusBottomRight:
		/* bottom right corner */
		dx = width - w->width - (x - hx) -
		     w->input.right - putplusGetPadRight (s);
		dy = height - w->height - (y - hy) -
		     w->input.bottom - putplusGetPadBottom (s);
		break;
	    case PutplusEmptyBottomRight:
		/* bottom right corner */
		hy = hy + w->input.bottom;
		hx = hx + w->input.right;
		dx = width - w->width - (x - hx) -
		     w->input.right - putplusGetPadRight (s);
		dy = height - w->height - (y - hy) -
		     w->input.bottom - putplusGetPadBottom (s);
		break;
	    case PutplusBottom:
		/* center of bottom edge */
		dx = (width / 2) - (w->width / 2) - (x - hx);
		dy = height - w->height - (y - hy) -
		     w->input.bottom - putplusGetPadBottom (s);
		break;
	    case PutplusEmptyBottom:
		/* center of bottom edge */
		hy = hy + w->input.bottom;
		dx = (width / 2) - (w->width / 2) - (x - hx);
		dy = height - w->height - (y - hy) -
		     w->input.bottom - putplusGetPadBottom (s);
		break;
	    case PutplusBottomLeft:
		/* bottom left corner */
		dx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		dy = height - w->height - (y - hy) -
		     w->input.bottom - putplusGetPadBottom (s);
		break;
	    case PutplusEmptyBottomLeft:
		/* bottom left corner */
		hy = hy + w->input.bottom;
		hx = hx - w->input.left;
		dx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		dy = height - w->height - (y - hy) -
		     w->input.bottom - putplusGetPadBottom (s);
		break;
	    case PutplusRestore:
		/* back to last position */
		dx = pw->lastX - x;
		dy = pw->lastY - y;
		break;
	    case PutplusViewport:
		{
		    int face, faceX, faceY, hDirection, vDirection;

		    /* get the face to move to from the options list */
		    face = getIntOptionNamed(option, nOption, "face", -1);

		    /* if it wasn't supplied, bail out */
		    if (face < 0)
			return FALSE;

		    /* split 1D face value into 2D x and y face */
		    faceX = face % s->hsize;
		    faceY = face / s->hsize;
		    if (faceY > s->vsize)
			faceY = s->vsize - 1;

		    /* take the shortest horizontal path to the
		       destination viewport */
		    hDirection = (faceX - s->x);
		    if (hDirection > s->hsize / 2)
			hDirection = (hDirection - s->hsize);
		    else if (hDirection < -s->hsize / 2)
			hDirection = (hDirection + s->hsize);

		    /* we need to do this for the vertical
		       destination viewport too */
		    vDirection = (faceY - s->y);
		    if (vDirection > s->vsize / 2)
			vDirection = (vDirection - s->vsize);
		    else if (vDirection < -s->vsize / 2)
			vDirection = (vDirection + s->vsize);

		    dx = s->width * hDirection;
		    dy = s->height * vDirection;
		    break;
		}
	    case PutplusViewportLeft:
		/* move to the viewport on the left */
		dx = -s->width;
		dy = 0;
		break;
	    case PutplusViewportRight:
		/* move to the viewport on the right */
		dx = s->width;
		dy = 0;
		break;
	    case PutplusViewportUp:
		/* move to the viewport above */
		dx = 0;
		dy = -s->height;
		break;
	    case PutplusViewportDown:
		/* move to the viewport below */
		dx = 0;
		dy = s->height;
		break;
	    case PutplusAbsolute:
		/* move the window to an exact position */
		if (px < 0)
		    /* account for a specified negative position,
		       like geometry without (-0) */
		    dx = px + s->width - w->width - x - w->input.right;
		else
		    dx = px - x + w->input.left;

		if (py < 0)
		    /* account for a specified negative position,
		       like geometry without (-0) */
		    dy = py + s->height - w->height - y - w->input.bottom;
		else
		    dy = py - y + w->input.top;
		break;
	    case PutplusRelative:
		/* move window by offset */
		dx = px;
		dy = py;
		break;
	    case PutplusPointer:
		{
		    /* move the window to the pointers position
		     * using the current quad of the screen to determine
		     * which corner to move to the pointer
		     */
		    int          rx, ry;
		    Window       root, child;
		    int          winX, winY;
		    unsigned int pMask;

		    /* get the pointers position from the X server */
		    if (XQueryPointer (d->display, w->id, &root, &child,
				       &rx, &ry, &winX, &winY, &pMask))
		    {
		        if (putplusGetWindowCenter (s))
			{
			        /* window center */
			        dx = rx - (w->width / 2) - x;
				dy = ry - (w->height / 2) - y;
			}
			else if (rx < s->workArea.width / 2 &&
				ry < s->workArea.height / 2)
			{
			        /* top left quad */
			        dx = rx - x + w->input.left;
				dy = ry - y + w->input.top;
			}
			else if (rx < s->workArea.width / 2 &&
				ry >= s->workArea.height / 2)
			{
			        /* bottom left quad */
			        dx = rx - x + w->input.left;
				dy = ry - w->height - y - w->input.bottom;
			}
			else if (rx >= s->workArea.width / 2 &&
				ry < s->workArea.height / 2)
			{
			        /* top right quad */
			        dx = rx - w->width - x - w->input.right;
				dy = ry - y + w->input.top;
			}
			else
			{
			        /* bottom right quad */
			        dx = rx - w->width - x - w->input.right;
				dy = ry - w->height - y - w->input.bottom;
			}
		    }
		    else
		    {
		        dx = dy = 0;
		    }
		}
		break;
	    default:
		/* if an unknown type is specified, do nothing */
		dx = dy = 0;
		break;
	    }

	    /* don't do anything if there is nothing to do */
	    if (dx != 0 || dy != 0)
	    {
		if (putplusGetAvoidOffscreen (s))
		{
		    /* avoids window borders offscreen, but allow full
		       viewport movements */
		    int inDx, dxBefore;
		    int inDy, dyBefore;

		    inDx = dxBefore = dx % s->width;
		    inDy = dyBefore = dy % s->height;

		    if ((-(x - hx) + w->input.left + putplusGetPadLeft (s)) > inDx)
		    {
			inDx = -(x - hx) + w->input.left + putplusGetPadLeft (s);
		    }
		    else if ((width - w->width - (x - hx) -
			      w->input.right - putplusGetPadRight (s)) < inDx)
		    {
			inDx = width - w->width - (x - hx) -
			       w->input.right - putplusGetPadRight (s);
		    }

		    if ((-(y - hy) + w->input.top + putplusGetPadTop (s)) > inDy)
		    {
			inDy = -(y - hy) + w->input.top + putplusGetPadTop (s);
		    }
		    else if ((height - w->height - (y - hy) - w->input.bottom -
			      putplusGetPadBottom (s)) < inDy)
		    {
			inDy = height - w->height - (y - hy) -
			       w->input.bottom - putplusGetPadBottom (s);
		    }

		    /* apply the change */
		    dx += inDx - dxBefore;
		    dy += inDy - dyBefore;
		}

		/* save the windows position in the saveMask
		 * this is used when unmaximizing the window
		 */
		if (w->saveMask & CWX)
		    w->saveWc.x += dx;

		if (w->saveMask & CWY)
		    w->saveWc.y += dy;

		/* Make sure everyting starts out at the windows
		   current position */
		pw->lastX = x;
		pw->lastY = y;

		pw->targetX = x + dx;
		pw->targetY = y + dy;

		/* mark for animation */
		pw->adjust = TRUE;
		ps->moreAdjust = TRUE;

		/* cause repainting */
		addWindowDamage (w);
	    }
	}
    }

    /* tell event.c handleEvent to not call XAllowEvents */
    return FALSE;
}

static PutplusType
putplusTypeFromString (char *type)
{
    if (strcasecmp (type, "absolute") == 0)
	return PutplusAbsolute;
    else if (strcasecmp (type, "relative") == 0)
	return PutplusRelative;
    else if (strcasecmp (type, "pointer") == 0)
	return PutplusPointer;
    else if (strcasecmp (type, "viewport") == 0)
	return PutplusViewport;
    else if (strcasecmp (type, "viewportleft") == 0)
	return PutplusViewportLeft;
    else if (strcasecmp (type, "viewportright") == 0)
	return PutplusViewportRight;
    else if (strcasecmp (type, "viewportup") == 0)
	return PutplusViewportUp;
    else if (strcasecmp (type, "viewportdown") == 0)
	return PutplusViewportDown;
    else if (strcasecmp (type, "restore") == 0)
	return PutplusRestore;
    else if (strcasecmp (type, "bottomleft") == 0)
	return PutplusBottomLeft;
    else if (strcasecmp (type, "left") == 0)
	return PutplusLeft;
    else if (strcasecmp (type, "topleft") == 0)
	return PutplusTopLeft;
    else if (strcasecmp (type, "top") == 0)
	return PutplusTop;
    else if (strcasecmp (type, "topright") == 0)
	return PutplusTopRight;
    else if (strcasecmp (type, "right") == 0)
	return PutplusRight;
    else if (strcasecmp (type, "bottomright") == 0)
	return PutplusBottomRight;
    else if (strcasecmp (type, "bottom") == 0)
	return PutplusBottom;
    else if (strcasecmp (type, "center") == 0)
	return PutplusCenter;
    else
	return PutplusUnknown;
}

static Bool
putplusToViewport (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int             nOption)
{
    int        face;
    CompOption o[4];

    /* get the face option */
    face = getIntOptionNamed (option, nOption, "face", -1);

    /* if it's not supplied, lets figure it out */
    if (face < 0)
    {
	PutplusDisplayOptions i;
	CompOption        *opt;

	i = PutplusDisplayOptionPutplusViewport1Key;

	while (i <= PutplusDisplayOptionPutplusViewport12Key)
	{
	    opt = putplusGetDisplayOption (d, i);
	    if (&opt->value.action == action)
	    {
		face = i - PutplusDisplayOptionPutplusViewport1Key;
		break;
	    }
	    i++;
	}
    }

    if (face < 0)
	return FALSE;

    /* setup the options for putInitiate */
    o[0].type    = CompOptionTypeInt;
    o[0].name    = "x";
    o[0].value.i = getIntOptionNamed (option, nOption, "x", 0);

    o[1].type    = CompOptionTypeInt;
    o[1].name    = "y";
    o[1].value.i = getIntOptionNamed (option, nOption, "y", 0);

    o[2].type    = CompOptionTypeInt;
    o[2].name    = "face";
    o[2].value.i = face;

    o[3].type    = CompOptionTypeInt;
    o[3].name    = "window";
    o[3].value.i = getIntOptionNamed (option, nOption, "window", 0);

    return putplusInitiateCommon (d, NULL, 0, o, 4, PutplusViewport,FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusInitiate (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    PutplusType type = PutplusUnknown;
    char    *typeString;

    typeString = getStringOptionNamed (option, nOption, "type", NULL);
    if (typeString)
	type = putplusTypeFromString (typeString);

    if (type == PutplusViewport)
	return putplusToViewport (d, action, state, option, nOption);
    else
	return putplusInitiateCommon (d, action, state, option, nOption, type,FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusViewportLeft (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusViewportLeft,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusViewportRight (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusViewportRight,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusViewportUp (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusViewportUp,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusViewportDown (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusViewportDown,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusRestore (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusRestore,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusPointer (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusPointer,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusCenter (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusCenter,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyCenter (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyCenter,
				  TRUE,TRUE,TRUE,TRUE);
}

static Bool
putplusLeft (CompDisplay     *d,
	     CompAction      *action,
	     CompActionState state,
	     CompOption      *option,
	     int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusLeft,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyLeft (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyLeft,
				  TRUE,FALSE,FALSE,FALSE);
}

static Bool
putplusTopLeft (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusTopLeft,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyTopLeft (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyTopLeft,
				  TRUE,FALSE,TRUE,FALSE);
}

static Bool
putplusTop (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusTop,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyTop (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyTop,
				  FALSE,FALSE,TRUE,FALSE);
}

static Bool
putplusTopRight (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusTopRight,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyTopRight (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int             nOption)
{
    return putplusInitiateCommon (d, action, state,
			      option, nOption, PutplusEmptyTopRight,
			      FALSE,TRUE,TRUE,FALSE);
}

static Bool
putplusRight (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusRight,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyRight (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyRight,
				  FALSE,TRUE,FALSE,FALSE);
}

static Bool
putplusBottomRight (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusBottomRight,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyBottomRight (CompDisplay     *d,
			 CompAction      *action,
			 CompActionState state,
			 CompOption      *option,
			 int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyBottomRight,
				  FALSE,TRUE,FALSE,TRUE);
}

static Bool
putplusBottom (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusBottom,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyBottom (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusEmptyBottom,
				  FALSE,FALSE,FALSE,TRUE);
}

static Bool
putplusBottomLeft (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    return putplusInitiateCommon (d, action, state,
				  option, nOption, PutplusBottomLeft,
				  FALSE,FALSE,FALSE,FALSE);
}

static Bool
putplusEmptyBottomLeft (CompDisplay     *d,
			CompAction      *action,
			CompActionState state,
			CompOption      *option,
			int             nOption)
{
    return putplusInitiateCommon (d, action, state,
			      option, nOption, PutplusEmptyBottomLeft,
			      TRUE,FALSE,FALSE,TRUE);
}

static void
putplusHandleEvent (CompDisplay *d,
		    XEvent      *event)
{
    PUTPLUS_DISPLAY (d);

    switch (event->type)
    {
	/* handle client events */
    case ClientMessage:
	/* accept the custom atom for putting windows */
	if (event->xclient.message_type == pd->compizPutplusWindowAtom)
	{
	    CompWindow *w;

	    w = findWindowAtDisplay(d, event->xclient.window);
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

		putplusInitiateCommon (w->screen->display, NULL, 0, opt, 5,
				   event->xclient.data.l[3],
				   FALSE,FALSE,FALSE,FALSE);
	    }
	}
	break;
    default:
	break;
    }

    UNWRAP (pd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (pd, d, handleEvent, putplusHandleEvent);
}

static Bool
putplusInitDisplay (CompPlugin  *p,
		    CompDisplay *d)
{
    PutplusDisplay *pd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    pd = malloc (sizeof (PutplusDisplay));
    if (!pd)
	return FALSE;

    pd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (pd->screenPrivateIndex < 0)
    {
	free (pd);
	return FALSE;
    }

    /* custom atom for client events */
    pd->compizPutplusWindowAtom = XInternAtom(d->display,
					      "_COMPIZ_PUTPLUS_WINDOW", 0);

    pd->lastWindow = None;
    pd->lastType   = PutplusUnknown;

    putplusSetPutplusViewportInitiate (d, putplusToViewport);
    putplusSetPutplusViewport1KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport2KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport3KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport4KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport5KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport6KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport7KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport8KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport9KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport10KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport11KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewport12KeyInitiate (d, putplusToViewport);
    putplusSetPutplusViewportLeftKeyInitiate (d, putplusViewportLeft);
    putplusSetPutplusViewportRightKeyInitiate (d, putplusViewportRight);
    putplusSetPutplusViewportUpKeyInitiate (d, putplusViewportUp);
    putplusSetPutplusViewportDownKeyInitiate (d, putplusViewportDown);
    putplusSetPutplusRestoreKeyInitiate (d, putplusRestore);
    putplusSetPutplusPointerKeyInitiate (d, putplusPointer);
    putplusSetPutplusRestoreButtonInitiate (d, putplusRestore);
    putplusSetPutplusPointerButtonInitiate (d, putplusPointer);
    putplusSetPutplusPutInitiate (d, putplusInitiate);
    putplusSetPutplusCenterKeyInitiate (d, putplusCenter);
    putplusSetPutplusLeftKeyInitiate (d, putplusLeft);
    putplusSetPutplusRightKeyInitiate (d, putplusRight);
    putplusSetPutplusTopKeyInitiate (d, putplusTop);
    putplusSetPutplusBottomKeyInitiate (d, putplusBottom);
    putplusSetPutplusTopleftKeyInitiate (d, putplusTopLeft);
    putplusSetPutplusToprightKeyInitiate (d, putplusTopRight);
    putplusSetPutplusBottomleftKeyInitiate (d, putplusBottomLeft);
    putplusSetPutplusBottomrightKeyInitiate (d, putplusBottomRight);
    putplusSetPutplusCenterButtonInitiate (d, putplusCenter);
    putplusSetPutplusLeftButtonInitiate (d, putplusLeft);
    putplusSetPutplusRightButtonInitiate (d, putplusRight);
    putplusSetPutplusTopButtonInitiate (d, putplusTop);
    putplusSetPutplusBottomButtonInitiate (d, putplusBottom);
    putplusSetPutplusTopleftButtonInitiate (d, putplusTopLeft);
    putplusSetPutplusToprightButtonInitiate (d, putplusTopRight);
    putplusSetPutplusBottomleftButtonInitiate (d, putplusBottomLeft);
    putplusSetPutplusBottomrightButtonInitiate (d, putplusBottomRight);

    /* Empty triggers */
    putplusSetPutplusEmptyCenterKeyInitiate (d, putplusEmptyCenter);
    putplusSetPutplusEmptyLeftKeyInitiate (d, putplusEmptyLeft);
    putplusSetPutplusEmptyRightKeyInitiate (d, putplusEmptyRight);
    putplusSetPutplusEmptyTopKeyInitiate (d, putplusEmptyTop);
    putplusSetPutplusEmptyBottomKeyInitiate (d, putplusEmptyBottom);
    putplusSetPutplusEmptyTopleftKeyInitiate (d, putplusEmptyTopLeft);
    putplusSetPutplusEmptyToprightKeyInitiate (d, putplusEmptyTopRight);
    putplusSetPutplusEmptyBottomleftKeyInitiate (d, putplusEmptyBottomLeft);
    putplusSetPutplusEmptyBottomrightKeyInitiate (d, putplusEmptyBottomRight);
    putplusSetPutplusEmptyCenterButtonInitiate (d, putplusEmptyCenter);
    putplusSetPutplusEmptyLeftButtonInitiate (d, putplusEmptyLeft);
    putplusSetPutplusEmptyRightButtonInitiate (d, putplusEmptyRight);
    putplusSetPutplusEmptyTopButtonInitiate (d, putplusEmptyTop);
    putplusSetPutplusEmptyBottomButtonInitiate (d, putplusEmptyBottom);
    putplusSetPutplusEmptyTopleftButtonInitiate (d, putplusEmptyTopLeft);
    putplusSetPutplusEmptyToprightButtonInitiate (d, putplusEmptyTopRight);
    putplusSetPutplusEmptyBottomleftButtonInitiate (d, putplusEmptyBottomLeft);
    putplusSetPutplusEmptyBottomrightButtonInitiate (d, putplusEmptyBottomRight);

    WRAP (pd, d, handleEvent, putplusHandleEvent);
    d->base.privates[displayPrivateIndex].ptr = pd;

    return TRUE;
}

static void
putplusFiniDisplay (CompPlugin  *p,
		    CompDisplay *d)
{
    PUTPLUS_DISPLAY (d);

    freeScreenPrivateIndex (d, pd->screenPrivateIndex);
    UNWRAP (pd, d, handleEvent);

    free (pd);
}

static Bool
putplusInitScreen (CompPlugin *p,
		   CompScreen *s)
{
    PutplusScreen *ps;

    PUTPLUS_DISPLAY (s->display);

    ps = malloc (sizeof (PutplusScreen));
    if (!ps)
	return FALSE;

    ps->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ps->windowPrivateIndex < 0)
    {
	free (ps);
	return FALSE;
    }

    /* initialize variables
     * bad stuff happens if we don't do this
     */
    ps->moreAdjust = FALSE;
    ps->grabIndex = 0;

    /* wrap the overloaded functions */
    WRAP (ps, s, preparePaintScreen, putplusPreparePaintScreen);
    WRAP (ps, s, donePaintScreen, putplusDonePaintScreen);
    WRAP (ps, s, paintOutput, putplusPaintOutput);
    WRAP (ps, s, paintWindow, putplusPaintWindow);

    s->base.privates[pd->screenPrivateIndex].ptr = ps;
    return TRUE;
}

static void
putplusFiniScreen (CompPlugin *p,
		   CompScreen *s)
{
    PUTPLUS_SCREEN (s);

    freeWindowPrivateIndex (s, ps->windowPrivateIndex);

    UNWRAP (ps, s, preparePaintScreen);
    UNWRAP (ps, s, donePaintScreen);
    UNWRAP (ps, s, paintOutput);
    UNWRAP (ps, s, paintWindow);

    free (ps);
}

static Bool
putplusInitWindow (CompPlugin *p,
		   CompWindow *w)
{
    PutplusWindow *pw;

    PUTPLUS_SCREEN (w->screen);

    pw = malloc (sizeof (PutplusWindow));
    if (!pw)
	return FALSE;

    /* initialize variables
     * I don't need to repeat it
     */
    pw->tx = pw->ty = pw->xVelocity = pw->yVelocity = 0.0f;
    pw->lastX = w->serverX;
    pw->lastY = w->serverY;
    pw->adjust = FALSE;

    w->base.privates[ps->windowPrivateIndex].ptr = pw;

    return TRUE;
}

static void
putplusFiniWindow (CompPlugin *p,
		   CompWindow *w)
{
    PUTPLUS_WINDOW (w);

    free (pw);
}

static CompBool
putplusInitObject (CompPlugin *p,
		   CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) putplusInitDisplay,
	(InitPluginObjectProc) putplusInitScreen,
	(InitPluginObjectProc) putplusInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
putplusFiniObject (CompPlugin *p,
	       CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) putplusFiniDisplay,
	(FiniPluginObjectProc) putplusFiniScreen,
	(FiniPluginObjectProc) putplusFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
putplusInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
putplusFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

/*
 * vTable tells the dl
 * what we offer
 */
CompPluginVTable putplusVTable = {
    "putplus",
    0,
    putplusInit,
    putplusFini,
    putplusInitObject,
    putplusFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
	return &putplusVTable;
}

#undef GET_PUTPLUS_DISPLAY
#undef PUTPLUS_DISPLAY
#undef GET_PUTPLUS_SCREEN
#undef PUTPLUS_SCREEN
#undef GET_PUTPLUS_WINDOW
#undef PUTPLUS_WINDOW
#undef PUTPLUS_ONLY_EMPTY
