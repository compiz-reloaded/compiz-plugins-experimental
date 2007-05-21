/*
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * Give credit where credit is due, keep the authors message below.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   - Diogo Ferreira (playerX) <diogo@beryl-project.org>
 *   - Danny Baumann <maniac@beryl-project.org>
 *
 *
 * Copyright (c) 2007 Diogo "playerX" Ferreira
 *
 * This wouldn't have been possible without:
 *   - Ideas from the compiz community (mainly throughnothing's)
 *   - David Reveman's work
 *
 * */

#include <stdlib.h>
#include <string.h>

#include <compiz.h>
#include "showdesktop_options.h"

#include <math.h>

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define OFF_LEFT(w) ((w)->width + (w)->input.right)
#define OFF_RIGHT(w) ((w)->input.left)
#define OFF_TOP(w) ((w)->height + (w)->input.bottom)
#define OFF_BOTTOM(w) ((w)->input.top)

#define MOVE_LEFT(w) ((WIN_X(w) + (WIN_W(w)/2)) < ((w)->screen->width/2))
#define MOVE_UP(w) ((WIN_Y(w) + (WIN_H(w)/2)) < ((w)->screen->height/2))

#define SD_STATE_OFF          0
#define SD_STATE_ACTIVATING   1
#define SD_STATE_ON           2
#define SD_STATE_DEACTIVATING 3

/* necessary plugin structs */
typedef struct _ShowdesktopPlacer
{
	int placed;
	int onScreenX, onScreenY;
	int offScreenX, offScreenY;
	int origViewportX;
	int origViewportY;
} ShowdesktopPlacer;

typedef struct _ShowdesktopDisplay
{
	int screenPrivateIndex;

	HandleEventProc handleEvent;
} ShowdesktopDisplay;

typedef struct _ShowdesktopScreen
{
	int windowPrivateIndex;

	PreparePaintScreenProc preparePaintScreen;
	PaintScreenProc paintScreen;
	DonePaintScreenProc donePaintScreen;
	PaintWindowProc paintWindow;
	EnterShowDesktopModeProc enterShowDesktopMode;
	LeaveShowDesktopModeProc leaveShowDesktopMode;

	int state;
	int moreAdjust;

	Bool ignoreNextTerminateEvent;
} ShowdesktopScreen;

typedef struct _ShowdesktopWindow
{
	int sid;
	int distance;

	ShowdesktopPlacer *placer;

	GLfloat xVelocity, yVelocity;
	GLfloat tx, ty;

	float delta;
	Bool adjust;
} ShowdesktopWindow;

/* shortcut macros, usually named X_DISPLAY, X_SCREEN and X_WINDOW
 * these might seem overly complicated but they are shortcuts so we don't have to access the privates arrays all the time
 * */
#define GET_SHOWDESKTOP_DISPLAY(d)				     \
    ((ShowdesktopDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define SD_DISPLAY(d)			   \
    ShowdesktopDisplay *sd = GET_SHOWDESKTOP_DISPLAY (d)

#define GET_SHOWDESKTOP_SCREEN(s, fd)					 \
    ((ShowdesktopScreen *) (s)->privates[(fd)->screenPrivateIndex].ptr)

#define SD_SCREEN(s)							\
    ShowdesktopScreen *ss = GET_SHOWDESKTOP_SCREEN (s, GET_SHOWDESKTOP_DISPLAY (s->display))

#define GET_SHOWDESKTOP_WINDOW(w, ss)					  \
    ((ShowdesktopWindow *) (w)->privates[(ss)->windowPrivateIndex].ptr)

#define SD_WINDOW(w)					       \
    ShowdesktopWindow *sw = GET_SHOWDESKTOP_WINDOW  (w,		       \
            GET_SHOWDESKTOP_SCREEN  (w->screen,	       \
                GET_SHOWDESKTOP_DISPLAY (w->screen->display)))
/* plugin private index */
static int displayPrivateIndex;


/* non interfacing code, aka the logic of the plugin */
static Bool isSDWin(CompWindow * w)
{
	if (!(*w->screen->focusWindow) (w))
		return FALSE;

	if (!matchEval(showdesktopGetWindowMatch(w->screen), w))
	    return FALSE;

	if (w->state & CompWindowStateSkipPagerMask)
		return FALSE;

	return TRUE;
}

static void setSkipPagerHint(CompWindow *w, Bool enable)
{
	unsigned int state = w->state;

    if (enable)
		state |= CompWindowStateSkipPagerMask;
    else
		state &= ~CompWindowStateSkipPagerMask;

	changeWindowState(w, state);
}

static void repositionSDPlacer(CompWindow * w, int oldState)
{
	SD_WINDOW(w);

	if (!sw->placer)
		return;

	if (oldState == SD_STATE_OFF)
	{
		sw->placer->onScreenX = w->attrib.x;
		sw->placer->onScreenY = w->attrib.y;
		sw->placer->origViewportX = w->screen->x;
		sw->placer->origViewportY = w->screen->y;
	}

	switch (showdesktopGetDirection(w->screen))
	{
	case DirectionUp:
		sw->placer->offScreenX = w->attrib.x;
		sw->placer->offScreenY = w->screen->workArea.y - OFF_TOP(w) + 
			                    showdesktopGetWindowPartSize(w->screen);
		break;
	case DirectionDown:
		sw->placer->offScreenX = w->attrib.x;
		sw->placer->offScreenY = w->screen->workArea.y + w->screen->workArea.height +
				                OFF_BOTTOM(w) - showdesktopGetWindowPartSize(w->screen);
		break;
	case DirectionLeft:
		sw->placer->offScreenX = w->screen->workArea.x - OFF_LEFT(w) + 
			                    showdesktopGetWindowPartSize(w->screen);
		sw->placer->offScreenY = w->attrib.y;
		break;
	case DirectionRight:
		sw->placer->offScreenX = w->screen->workArea.x + w->screen->workArea.width +
				                OFF_RIGHT(w) - showdesktopGetWindowPartSize(w->screen);
		sw->placer->offScreenY = w->attrib.y;
		break;
	case DirectionUpDown:
		sw->placer->offScreenX = w->attrib.x;
		if (MOVE_UP(w))
			sw->placer->offScreenY = w->screen->workArea.y - OFF_TOP(w) + 
				                    showdesktopGetWindowPartSize(w->screen);
		else
			sw->placer->offScreenY = w->screen->workArea.y +
					                w->screen->workArea.height + OFF_BOTTOM(w) -
					                showdesktopGetWindowPartSize(w->screen);
		break;
	case DirectionLeftRight:
		sw->placer->offScreenY = w->attrib.y;
		if (MOVE_LEFT(w))
			sw->placer->offScreenX = w->screen->workArea.x - OFF_LEFT(w) + 
				                    showdesktopGetWindowPartSize(w->screen);
		else
			sw->placer->offScreenX = w->screen->workArea.x +
					                w->screen->workArea.width + OFF_RIGHT(w) -
					                showdesktopGetWindowPartSize(w->screen);
		break;
	case DirectionToCorners:
		if (MOVE_LEFT(w))
			sw->placer->offScreenX = w->screen->workArea.x - OFF_LEFT(w) + 
				                    showdesktopGetWindowPartSize(w->screen);
		else
			sw->placer->offScreenX = w->screen->workArea.x +
					                w->screen->workArea.width + OFF_RIGHT(w) -
					                showdesktopGetWindowPartSize(w->screen);
		if (MOVE_UP(w))
			sw->placer->offScreenY = w->screen->workArea.y - OFF_TOP(w) + 
				                    showdesktopGetWindowPartSize(w->screen);
		else
			sw->placer->offScreenY = w->screen->workArea.y +
					                w->screen->workArea.height + OFF_BOTTOM(w) -
					                showdesktopGetWindowPartSize(w->screen);
		break;
	default:
		break;
	}
}

static int prepareSDWindows(CompScreen * s, int oldState)
{
	CompWindow *w;
	CompWindow *desktopWindow;
	int count = 0;

	desktopWindow = 0;

	for (w = s->windows; w; w = w->next)
	{
		SD_WINDOW(w);

		if (getWindowType(s->display, w->id) == CompWindowTypeDesktopMask)
			desktopWindow = w;

		if (!isSDWin(w))
			continue;

		if (!sw->placer)
			sw->placer = malloc(sizeof(ShowdesktopPlacer));

		repositionSDPlacer(w, oldState);

		sw->placer->placed = TRUE;
		sw->adjust = TRUE;
		w->inShowDesktopMode = TRUE;
		setSkipPagerHint(w, TRUE);

		if (sw->tx)
			sw->tx -= (sw->placer->onScreenX - sw->placer->offScreenX);
		if (sw->ty)
			sw->ty -= (sw->placer->onScreenY - sw->placer->offScreenY);

		moveWindow(w, 
				   sw->placer->offScreenX - w->attrib.x,
   				   sw->placer->offScreenY - w->attrib.y, 
				   TRUE, TRUE);
		syncWindowPosition(w);

		count++;
	}

	if (desktopWindow)
		activateWindow(desktopWindow);

	return count;
}

/* plugin initialization */

static Bool showdesktopInit(CompPlugin * p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();

	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

/* plugin finalization */
static void showdesktopFini(CompPlugin * p)
{

	if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex(displayPrivateIndex);
}

/* adjust velocity for each animation step (adapted from the scale plugin) */
static int adjustSDVelocity(CompWindow * w)
{
	float dx, dy, adjust, amount;
	float x1, y1;
	float baseX, baseY;

	SD_WINDOW(w);
	SD_SCREEN(w->screen);

	x1 = y1 = 0.0;

	if (ss->state == SD_STATE_ACTIVATING)
	{
		x1 = sw->placer->offScreenX;
		y1 = sw->placer->offScreenY;
		baseX = sw->placer->onScreenX;
		baseY = sw->placer->onScreenY;
	}
	else 
	{
		x1 = sw->placer->onScreenX;
		y1 = sw->placer->onScreenY;
		baseX = sw->placer->offScreenX;
		baseY = sw->placer->offScreenY;
	}

	dx = x1 - (baseX + sw->tx);

	adjust = dx * 0.15f;
	amount = fabs(dx) * 1.5f;
	if (amount < 0.5f)
		amount = 0.5f;
	else if (amount > 5.0f)
		amount = 5.0f;

	sw->xVelocity = (amount * sw->xVelocity + adjust) / (amount + 1.0f);

	dy = y1 - (baseY + sw->ty);

	adjust = dy * 0.15f;
	amount = fabs(dy) * 1.5f;
	if (amount < 0.5f)
		amount = 0.5f;
	else if (amount > 5.0f)
		amount = 5.0f;

	sw->yVelocity = (amount * sw->yVelocity + adjust) / (amount + 1.0f);

	if (fabs(dx) < 0.1f && fabs(sw->xVelocity) < 0.2f &&
		fabs(dy) < 0.1f && fabs(sw->yVelocity) < 0.2f)
	{
		sw->xVelocity = sw->yVelocity = 0.0f;
		sw->tx = x1 - baseX;
		sw->ty = y1 - baseY;

		return 0;
	}
	return 1;
}

/* this function gets called periodically (about every 15ms on this machine),
 * animation takes place here */
static void
showdesktopPreparePaintScreen(CompScreen * s, int msSinceLastPaint)
{
	SD_SCREEN(s);

	UNWRAP(ss, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, msSinceLastPaint);
	WRAP(ss, s, preparePaintScreen, showdesktopPreparePaintScreen);

	if ((ss->state == SD_STATE_ACTIVATING) ||
		(ss->state == SD_STATE_DEACTIVATING))
	{
		CompWindow *w;
		int steps;
		float amount, chunk;

		amount = msSinceLastPaint * 0.05f * showdesktopGetSpeed(s);
		steps = amount / (0.5f * showdesktopGetTimestep(s));
		if (!steps)
			steps = 1;
		chunk = amount / (float)steps;

		while (steps--)
		{
			ss->moreAdjust = 0;

			for (w = s->windows; w; w = w->next)
			{
				SD_WINDOW(w);

				if (sw->adjust)
				{
					sw->adjust = adjustSDVelocity(w);

					ss->moreAdjust |= sw->adjust;

					sw->tx += sw->xVelocity * chunk;
					sw->ty += sw->yVelocity * chunk;
				}
			}
			if (!ss->moreAdjust)
				break;
		}

	}
}

static Bool
showdesktopPaintScreen(CompScreen * s, 
					   const ScreenPaintAttrib * sAttrib,
			   		   const CompTransform  *transform, 
					   Region region,  int output, unsigned int mask)
{
	Bool status;

	SD_SCREEN(s);

	if ((ss->state == SD_STATE_ACTIVATING) || (ss->state == SD_STATE_DEACTIVATING))
		mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

	UNWRAP(ss, s, paintScreen);
	status = (*s->paintScreen) (s, sAttrib, transform, region, output, mask);
	WRAP(ss, s, paintScreen, showdesktopPaintScreen);

	return status;
}

/* this one gets called after the one above and periodically,
 * here the plugin checks if windows reached the end */
static void showdesktopDonePaintScreen(CompScreen * s)
{
	SD_SCREEN(s);

	if (ss->moreAdjust)
	{
		damageScreen(s);
	}
	else
	{
		if ((ss->state == SD_STATE_ACTIVATING) || 
			(ss->state == SD_STATE_DEACTIVATING))
		{
			CompWindow *w;
			if (ss->state == SD_STATE_ACTIVATING)
			{
				ss->state = SD_STATE_ON;
			}
			else 
			{
				Bool inSDMode = FALSE;

				for (w = s->windows; w; w = w->next)
				{
					if (w->inShowDesktopMode)
						inSDMode = TRUE;
					else
					{
						SD_WINDOW(w);
						if (sw->placer)
						{
							free(sw->placer);
							sw->placer = NULL;
						}
					}
				}

				if (inSDMode)
					ss->state = SD_STATE_ON;
				else
					ss->state = SD_STATE_OFF;
			}

			damageScreen(s);
		}
	}

	UNWRAP(ss, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP(ss, s, donePaintScreen, showdesktopDonePaintScreen);
}

static Bool showdesktopPaintWindow(CompWindow *w, 
								   const WindowPaintAttrib *attrib,
						   		   const CompTransform *transform, 
								   Region region, unsigned int mask)
{
	SD_SCREEN(w->screen);
	Bool status;

	if ((ss->state == SD_STATE_ACTIVATING) || (ss->state == SD_STATE_DEACTIVATING))
	{
		SD_WINDOW(w);

		CompTransform wTransform = *transform;
		WindowPaintAttrib wAttrib = *attrib;

		if (sw->adjust)
		{
			float offsetX, offsetY;

			offsetX = (ss->state == SD_STATE_DEACTIVATING) ? 
					(sw->placer->offScreenX - sw->placer->onScreenX) :
					(sw->placer->onScreenX - sw->placer->offScreenX);
			offsetY = (ss->state == SD_STATE_DEACTIVATING) ? 
					(sw->placer->offScreenY - sw->placer->onScreenY) :
					(sw->placer->onScreenY - sw->placer->offScreenY);

			wAttrib.opacity = OPAQUE;

			mask |= PAINT_WINDOW_TRANSFORMED_MASK;

			matrixTranslate (&wTransform, w->attrib.x, w->attrib.y, 0.0f);
			matrixScale (&wTransform, 1.0f, 1.0f, 0.0f);
			matrixTranslate (&wTransform, sw->tx + offsetX - w->attrib.x, 
					sw->ty + offsetY - w->attrib.y, 0.0f);
		}

		UNWRAP(ss, w->screen, paintWindow);
		status = (*w->screen->paintWindow) (w, &wAttrib, &wTransform, region, mask);
		WRAP(ss, w->screen, paintWindow, showdesktopPaintWindow);
	}
	else if (ss->state == SD_STATE_ON)
	{
		WindowPaintAttrib wAttrib = *attrib;

		if (w->inShowDesktopMode)
			wAttrib.opacity = wAttrib.opacity * showdesktopGetWindowOpacity(w->screen);

		UNWRAP(ss, w->screen, paintWindow);
		status = (*w->screen->paintWindow) (w, &wAttrib, transform, region, mask);
		WRAP(ss, w->screen, paintWindow, showdesktopPaintWindow);
	}
	else
	{
		UNWRAP(ss, w->screen, paintWindow);
		status = (*w->screen->paintWindow) (w, attrib, transform, region, mask);
		WRAP(ss, w->screen, paintWindow, showdesktopPaintWindow);
	}

	return status;
}

static void showdesktopHandleEvent(CompDisplay *d, XEvent *event)
{
	SD_DISPLAY(d);

	switch(event->type)
	{
		case ButtonPress:
			{
				CompScreen *s = findScreenAtDisplay(d, event->xbutton.root);
				if (s)
				{
					SD_SCREEN(s);
					if (ss->state != SD_STATE_OFF)
						event->xbutton.window = None;
				}
			}
			break;
		case ClientMessage:
			if ((event->xclient.message_type == d->restackWindowAtom) ||
				(event->xclient.message_type == d->moveResizeWindowAtom))
			{
				CompWindow *w = findWindowAtDisplay(d, event->xclient.window);
				if (w)
				{
					SD_SCREEN(w->screen);
					if (ss->state != SD_STATE_OFF)
						event->xclient.window = None;
				}
			}
			break;
		case PropertyNotify:
			if (event->xproperty.atom == d->desktopViewportAtom)
			{
				CompScreen *s = findScreenAtDisplay(d, event->xproperty.window);
				if (s)
				{
					SD_SCREEN(s);

					if ((ss->state == SD_STATE_ON) || (ss->state == SD_STATE_ACTIVATING))
						(*s->leaveShowDesktopMode)(s, NULL);
				}
			}
			break;
	}

	UNWRAP(sd, d, handleEvent);
	(*d->handleEvent)(d, event);
	WRAP(sd, d, handleEvent, showdesktopHandleEvent);
}

static void showdesktopEnterShowDesktopMode(CompScreen *s)
{
	SD_SCREEN(s);
	int count = 0;

	if (ss->state == SD_STATE_OFF || ss->state == SD_STATE_DEACTIVATING)
	{
		count = prepareSDWindows(s, ss->state);
		if (count > 0)
		{
			XSetInputFocus(s->display->display, s->root,
						   RevertToPointerRoot, CurrentTime);
			ss->state = SD_STATE_ACTIVATING;
			damageScreen(s);
		}
	}

	UNWRAP(ss, s, enterShowDesktopMode);
	(*s->enterShowDesktopMode)(s);
	WRAP(ss, s, enterShowDesktopMode, showdesktopEnterShowDesktopMode);
}

static void showdesktopLeaveShowDesktopMode(CompScreen *s, CompWindow *w)
{
	SD_SCREEN(s);

	if (ss->state != SD_STATE_OFF)
	{
		CompWindow *cw;

		for (cw = s->windows; cw; cw = cw->next)
		{
			SD_WINDOW(cw);

			if (w && (w->id != cw->id))
				continue;

			if (sw->placer && sw->placer->placed)
			{
				sw->adjust = TRUE;
				sw->placer->placed = FALSE;

				/* adjust onscreen position to 
				   handle viewport changes
				 */
				sw->tx += (sw->placer->onScreenX - sw->placer->offScreenX);
				sw->ty += (sw->placer->onScreenY - sw->placer->offScreenY);

				sw->placer->onScreenX += (sw->placer->origViewportX - 
						cw->screen->x) * cw->screen->width;
				sw->placer->onScreenY += (sw->placer->origViewportY -
						cw->screen->y) * cw->screen->height;

				moveWindow(cw, sw->placer->onScreenX - cw->attrib.x,
						sw->placer->onScreenY - cw->attrib.y, TRUE, TRUE);
				syncWindowPosition(cw);
		
				setSkipPagerHint(cw, FALSE);
				cw->inShowDesktopMode = FALSE;
			}
		}
		ss->state = SD_STATE_DEACTIVATING;
		damageScreen(s);
	}

	UNWRAP(ss, s, leaveShowDesktopMode);
	(*s->leaveShowDesktopMode)(s, w);
	WRAP(ss, s, leaveShowDesktopMode, showdesktopLeaveShowDesktopMode);
}

/* display initialization */

static Bool showdesktopInitDisplay(CompPlugin * p, CompDisplay * d)
{
	ShowdesktopDisplay *sd;

	sd = malloc(sizeof(ShowdesktopDisplay));	/* allocate the display */
	if (!sd)
		return FALSE;

	sd->screenPrivateIndex = allocateScreenPrivateIndex(d);
	if (sd->screenPrivateIndex < 0)
	{
		free(sd);
		return FALSE;
	}

	WRAP(sd, d, handleEvent, showdesktopHandleEvent);

	d->privates[displayPrivateIndex].ptr = sd;

	return TRUE;
}

static void showdesktopFiniDisplay(CompPlugin * p, CompDisplay * d)
{
	SD_DISPLAY(d);

	freeScreenPrivateIndex(d, sd->screenPrivateIndex);

	UNWRAP(sd, d, handleEvent);

	free(sd);
}

static Bool showdesktopInitScreen(CompPlugin * p, CompScreen * s)
{
	ShowdesktopScreen *ss;

	SD_DISPLAY(s->display);

	ss = malloc(sizeof(ShowdesktopScreen));
	if (!ss)
		return FALSE;

	ss->windowPrivateIndex = allocateWindowPrivateIndex(s);
	if (ss->windowPrivateIndex < 0)
	{
		free(ss);
		return FALSE;
	}

	ss->state = SD_STATE_OFF;
	ss->moreAdjust = 0;
	ss->ignoreNextTerminateEvent = FALSE;

	WRAP(ss, s, preparePaintScreen, showdesktopPreparePaintScreen);
	WRAP(ss, s, paintScreen, showdesktopPaintScreen);
	WRAP(ss, s, donePaintScreen, showdesktopDonePaintScreen);
	WRAP(ss, s, paintWindow, showdesktopPaintWindow);
	WRAP(ss, s, enterShowDesktopMode, showdesktopEnterShowDesktopMode);
	WRAP(ss, s, leaveShowDesktopMode, showdesktopLeaveShowDesktopMode);

	s->privates[sd->screenPrivateIndex].ptr = ss;

	return TRUE;

}

/* Free screen resources */
static void showdesktopFiniScreen(CompPlugin * p, CompScreen * s)
{
	SD_SCREEN(s);

	UNWRAP(ss, s, preparePaintScreen);
	UNWRAP(ss, s, paintScreen);
	UNWRAP(ss, s, donePaintScreen);
	UNWRAP(ss, s, paintWindow);
	UNWRAP(ss, s, enterShowDesktopMode);
	UNWRAP(ss, s, leaveShowDesktopMode);

	freeWindowPrivateIndex(s, ss->windowPrivateIndex);

	free(ss);
}


/* window init */
static Bool showdesktopInitWindow(CompPlugin * p, CompWindow * w)
{
	ShowdesktopWindow *sw;

	SD_SCREEN(w->screen);

	sw = malloc(sizeof(ShowdesktopWindow));
	if (!sw)
		return FALSE;

	sw->tx = sw->ty = 0.0f;
	sw->adjust = FALSE;
	sw->xVelocity = sw->yVelocity = 0.0f;
	sw->delta = 1.0f;
	sw->placer = NULL;

	w->privates[ss->windowPrivateIndex].ptr = sw;

	return TRUE;
}

/* Free window resources */
static void showdesktopFiniWindow(CompPlugin * p, CompWindow * w)
{
	SD_WINDOW(w);

	free(sw);
}

static int 
showdesktopGetVersion (CompPlugin *plugin, int version)
{
	return ABIVERSION;
}

/* plugin vtable */
static CompPluginVTable showdesktopVTable = {
	"showdesktop",
	showdesktopGetVersion,
	0,
	showdesktopInit,
	showdesktopFini,
	showdesktopInitDisplay,
	showdesktopFiniDisplay,
	showdesktopInitScreen,
	showdesktopFiniScreen,
	showdesktopInitWindow,
	showdesktopFiniWindow,
	NULL,
	NULL,
	NULL,
	NULL,
	0,							/* deps */
	0,							/* sizeof (deps) / sizeof (deps[0]) */
	0,
	0
};

/* send plugin info */
CompPluginVTable *getCompPluginInfo(void)
{
	return &showdesktopVTable;
}
