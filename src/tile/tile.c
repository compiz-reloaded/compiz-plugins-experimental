/**
 *
 * Compiz tile plugin
 *
 * tile.c
 *
 * Copyright (c) 2006 Atie H. <atie.at.matrix@gmail.com>
 * Copyright (c) 2006 Michal Fojtik <pichalsi(at)gmail.com>
 * Copyright (c) 2007 Danny Baumann <maniac@beryl-project.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * TODO
 *	- change vertical and horizontal tiling to similar behavior as Left
 *	- fix bugs
 *	- make vertical and horizontal maximization be saved when tiling
 *
 **/

#include <string.h>
#include <math.h>
#include <compiz-core.h>
#include "tile_options.h"

static int displayPrivateIndex = 0;

typedef enum {
    NoAnimation = 0,
    Animating,
    AnimationDone
} WindowAnimationType;

typedef struct _TileDisplay {
    int screenPrivateIndex;
} TileDisplay;

typedef struct _TileScreen {
    int windowPrivateIndex;

    int grabIndex;
    int oneDuration; // duration of animation for one window
    int msResizing; // number of ms elapsed from start of resizing animation

    TileTileToggleTypeEnum tileType;

    PaintWindowProc        paintWindow;
    WindowResizeNotifyProc windowResizeNotify;
    PreparePaintScreenProc preparePaintScreen;
    PaintScreenProc        paintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc        paintOutput;
} TileScreen;

typedef struct _TileWindow {
    Bool isTiled;

    XRectangle   savedCoords;
    XRectangle   prevCoords;
    XRectangle   newCoords;
    unsigned int savedMaxState;
    Bool         savedValid;

    Bool needConfigure;
    Bool alreadyResized;

    WindowAnimationType animationType;
    unsigned int        animationNum;

    GLushort outlineColor[3];
} TileWindow;

#define GET_TILE_DISPLAY(d) \
    ((TileDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define TILE_DISPLAY(d) \
    TileDisplay *td = GET_TILE_DISPLAY (d)

#define GET_TILE_SCREEN(s, td) \
    ((TileScreen *) (s)->base.privates[(td)->screenPrivateIndex].ptr)
#define TILE_SCREEN(s) \
    TileScreen *ts = GET_TILE_SCREEN (s, GET_TILE_DISPLAY (s->display))

#define GET_TILE_WINDOW(w, ts) \
    ((TileWindow *) (w)->base.privates[(ts)->windowPrivateIndex].ptr)
#define TILE_WINDOW(w) \
    TileWindow *tw = GET_TILE_WINDOW (w, \
		     GET_TILE_SCREEN (w->screen, \
		     GET_TILE_DISPLAY (w->screen->display)))

static Bool placeWin (CompWindow *w, int x, int y, int width, int height);
static Bool tileSetNewWindowSize (CompWindow *w);

/* window painting function, draws animation */
static Bool
tilePaintWindow(CompWindow              *w,
		const WindowPaintAttrib *attrib,
		const CompTransform     *transform,
		Region                  region,
		unsigned int            mask)
{
    CompScreen *s = w->screen;
    Bool status;
    Bool dontDraw = FALSE;

    TILE_WINDOW (w);
    TILE_SCREEN (s);

    if (tw->animationType != NoAnimation)
    {
	WindowPaintAttrib wAttrib = *attrib;
	CompTransform     wTransform = *transform;
	float             progress;

	progress = (float)ts->msResizing /
	           (float)tileGetAnimationDuration (s->display);

	switch (tileGetAnimateType (s->display))
	{
	    /*
	       Drop animation
	       */
	    case AnimateTypeDropFromTop:
		matrixRotate (&wTransform,
			      (progress * 100.0f) - 100.0f,
			      0.0f, 0.0f, 1.0f);
		mask |= PAINT_WINDOW_TRANSFORMED_MASK;
		break;

	    /*
	       Zoom animation
	       */
	    case AnimateTypeZoom:
		matrixTranslate (&wTransform, 0, 0, progress - 1.0f);
		mask |= PAINT_WINDOW_TRANSFORMED_MASK;
		break;

	    /*
	       Slide animation
	       */
	    case AnimateTypeSlide:
		if (progress < 0.75f)
		    wAttrib.opacity /= 2;
		else
		    wAttrib.opacity *= (0.5f + 2 * (progress - 0.75f));

		if (ts->msResizing > tw->animationNum * ts->oneDuration)
		{
		    /* animation finished */
		    tw->animationType = AnimationDone;
		}
		else if (ts->msResizing > (tw->animationNum - 1) *
			 ts->oneDuration)
		{
		    int thisDur; /* ms spent animating this window */
		    thisDur = ts->msResizing % ts->oneDuration;

		    if (tw->animationNum % 2)
			matrixTranslate (&wTransform,
					 s->width - s->width *
					 (float)thisDur / ts->oneDuration,
					 0, 0);
		    else
			matrixTranslate (&wTransform,
					 -s->width + s->width *
					 (float)thisDur / ts->oneDuration,
					 0, 0);

		    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
		}
		else
		    dontDraw = TRUE;
		break;
	    /*
	       Outline animation
	       */
	    case AnimateTypeFilledOutline:
		dontDraw = TRUE;
		break;

	    /*
	       Fade animation
	       */
	    case AnimateTypeFade:
		// first half of the animation, fade out
		if (progress < 0.4f)
		    wAttrib.opacity -= wAttrib.opacity * progress / 0.4f;
		else
		{
		    if (progress > 0.6f && tw->alreadyResized)
		    {
			// second half of animation, fade in
			wAttrib.opacity *= (progress - 0.6f) / 0.4f;
		    }
		    else
		    {
			if (tw->needConfigure)
			    tileSetNewWindowSize (w);
			dontDraw = TRUE;
		    }
		}
		break;

	    default:
		break;
	}

	if (dontDraw)
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

	UNWRAP (ts, s, paintWindow);
	status = (*s->paintWindow) (w, &wAttrib, &wTransform, region, mask);
	WRAP (ts, s, paintWindow, tilePaintWindow);
    }
    else // paint window as always
    {
	UNWRAP (ts, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ts, s, paintWindow, tilePaintWindow);
    }

    return status;
}

static void
tilePreparePaintScreen (CompScreen *s,
			int        msSinceLastPaint)
{
    TILE_SCREEN (s);

    // add spent time
    if (ts->grabIndex)
	ts->msResizing += msSinceLastPaint;

    UNWRAP (ts, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ts, s, preparePaintScreen, tilePreparePaintScreen);
}

static void
tilePaintScreen (CompScreen   *s,
		 CompOutput   *outputs,
		 int          numOutputs,
		 unsigned int mask)
{
    TILE_SCREEN (s);

    if (ts->grabIndex)
    {
	outputs = &s->fullscreenOutput;
	numOutputs = 1;
    }

    UNWRAP (ts, s, paintScreen);
    (*s->paintScreen) (s, outputs, numOutputs, mask);
    WRAP (ts, s, paintScreen, tilePaintScreen);
}

static void
tileDonePaintScreen (CompScreen *s)
{
    TILE_SCREEN (s);

    if (ts->grabIndex)
    {
	if (ts->msResizing > tileGetAnimationDuration (s->display))
	{
	    CompWindow *w;
	    for (w = s->windows; w; w = w->next)
	    {
		TILE_WINDOW (w);
		tw->animationType = NoAnimation;
	    }

	    ts->msResizing = 0;

	    removeScreenGrab (s, ts->grabIndex, NULL);
	    ts->grabIndex = 0;
	}

	damageScreen (s);
    }

    UNWRAP (ts, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ts, s, donePaintScreen, tileDonePaintScreen);
}

static Bool
tilePaintOutput (CompScreen              *s,
		 const ScreenPaintAttrib *sa,
		 const CompTransform	 *transform,
		 Region                  region,
		 CompOutput              *output,
		 unsigned int            mask)
{
    Bool status;

    TILE_SCREEN (s);

    if (ts->grabIndex)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (ts, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ts, s, paintOutput, tilePaintOutput);

    /* Check if animation is enabled, there is resizing
       on screen and only outline should be drawn */

    if (ts->grabIndex && (output->id == ~0) &&
	(tileGetAnimateType (s->display) == AnimateTypeFilledOutline))
    {
	CompWindow    *w;
	CompTransform sTransform = *transform;
	float         animationDuration = tileGetAnimationDuration (s->display);
	int           x, y, width, height;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	glLineWidth (4.0f);

	for (w = s->windows; w; w = w->next)
	{
	    TILE_WINDOW (w);

	    if (tw->animationType == Animating)
	    {
		/* Coordinate = start +            speed          * elapsedTime
		   Coordinate = start + (target - start)/interval * elapsedTime
		   Draw outline */

		x      = tw->prevCoords.x - w->input.left +
		         (((float)(w->attrib.x - tw->prevCoords.x)) *
			  ts->msResizing / animationDuration);
		y      = tw->prevCoords.y - w->input.top +
		         (((float)(w->attrib.y - tw->prevCoords.y)) *
			  ts->msResizing / animationDuration);
		width  = tw->prevCoords.width + w->input.left + w->input.right +
		         (((float)(w->attrib.width - tw->prevCoords.width)) *
			  ts->msResizing / animationDuration);
		height = tw->prevCoords.height +
		         w->input.top + w->input.bottom +
			 (((float)(w->attrib.height - tw->prevCoords.height)) *
			  ts->msResizing / animationDuration);

		glColor3us (tw->outlineColor[0] * 0.66,
			    tw->outlineColor[1] * 0.66,
			    tw->outlineColor[2] * 0.66);
		glRecti (x, y + height, x + width, y);

		glColor3usv (tw->outlineColor);

		glBegin (GL_LINE_LOOP);
		glVertex3f (x, y, 0.0f);
		glVertex3f (x + width, y, 0.0f);
		glVertex3f (x + width, y + height, 0.0f);
		glVertex3f (x, y + height, 0.0f);
		glEnd ();

		glColor4usv (defaultColor);
	    }
	}

	glPopMatrix ();
	glColor4usv (defaultColor);
	glLineWidth (1.0f);
    }

    return status;
}

/* Resize notify used when windows are tiled horizontally or vertically */
static void
tileResizeNotify (CompWindow *w,
		  int        dx,
		  int        dy,
		  int        dwidth,
		  int        dheight)
{
    TILE_SCREEN (w->screen);
    TILE_WINDOW (w);

    UNWRAP (ts, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify) (w, dx, dy, dwidth, dheight);
    WRAP (ts, w->screen, windowResizeNotify, tileResizeNotify);

    if (!tw->alreadyResized)
    {
	tw->alreadyResized = TRUE;
	return;
    }

    /* Dont do anything if joining is disabled or windows are being resized */
    if (tileGetTileJoin (w->screen->display) && !ts->grabIndex)
    {
	CompWindow *prev = NULL, *next = NULL, *cw;
	Bool       windowSeen = FALSE;

	/* determine previous and next tiled window */
	for (cw = w->screen->reverseWindows; cw; cw = cw->prev)
	{
	    if (windowSeen)
	    {
		next = cw;
		break;
	    }
	    else
	    {
		if (cw != w)
		    prev = cw;
		else
		    windowSeen = TRUE;
	    }
	}

	switch (ts->tileType)
	{
	case TileToggleTypeTile:
	    if (prev)
		placeWin (prev,
			  prev->attrib.x, prev->attrib.y,
			  w->attrib.x - prev->attrib.x -
			  w->input.left - prev->input.right,
			  prev->height);

	    if (next)
	    {
		int currentX;
		currentX = w->attrib.x + w->width +
		           w->input.right + next->input.left;
		placeWin (next, currentX, next->attrib.y,
			  next->width + next->attrib.x - currentX,
			  next->height);
	    }
	    break;

	case TileToggleTypeTileHorizontally:
	    if (prev)
		placeWin (prev,
			  prev->attrib.x, prev->attrib.y,
			  prev->width,
			  w->attrib.y - prev->attrib.y -
			  w->input.top - prev->input.bottom);

	    if (next)
	    {
		int currentY;
		currentY = w->attrib.y + w->height +
		           w->input.bottom + next->input.top;
		placeWin (next, next->attrib.x,
			  currentY, next->width,
			  next->height + next->attrib.y - currentY);
	    }
	    break;
	case TileToggleTypeLeft:
	    if (!next && prev && dwidth) /* last window - on the left */
	    {
		XRectangle workArea;
		int        currentX;

		workArea = w->screen->workArea;

		for (cw = w->screen->windows; cw; cw = cw->next)
		{
		    TILE_WINDOW(cw);

		    if (!tw->isTiled || (cw->id == w->id))
			continue;

		    currentX = workArea.x + w->serverX +
			       w->serverWidth + w->input.right + cw->input.left;

		    placeWin (cw, currentX, cw->attrib.y,
			      workArea.width - currentX - w->input.right,
			      cw->attrib.height);
		}
	    }
	    else if (next) /* windows on the right */
	    {
		XRectangle workArea;
		Bool       first = TRUE;

		workArea = w->screen->workArea;

		for (cw = w->screen->windows; cw; cw = cw->next)
		{
		    TILE_WINDOW (cw);

		    if (!tw->isTiled || (cw->id == w->id))
			continue;

		    if (first)
		    {
			placeWin (cw,
				  workArea.x + cw->input.left, cw->attrib.y,
				  w->serverX - w->input.left -
				  cw->input.left - cw->input.right - workArea.x,
				  cw->attrib.height);

			first = FALSE;
		    }
		    else
		    {
			int x = cw->attrib.x;
			int y = cw->attrib.y;
			int width = cw->attrib.width;
			int height = cw->attrib.height;

			if (prev && (cw->id == prev->id))
			    height = w->serverY - cw->attrib.y -
				     w->input.top - cw->input.bottom;
			else if (next && (cw->id == next->id))
			    y = w->serverY + w->serverHeight +
				w->input.bottom + cw->input.top;

			x = w->serverX;
			width = workArea.width + workArea.x -
			        w->serverX - w->input.right;

			placeWin (cw, x, y, width, height);
		    }
		}
	    }
	    break;

	default:
	    break;
	}
    }
}

static Bool
tileInitScreen (CompPlugin *p,
		CompScreen *s)
{
    TileScreen *ts;

    TILE_DISPLAY (s->display);

    ts = (TileScreen *) calloc (1, sizeof (TileScreen));

    ts->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ts->windowPrivateIndex < 0)
    {
	free (ts);
	return FALSE;
    }
    srand (time (0));

    s->base.privates[td->screenPrivateIndex].ptr = ts;

    ts->grabIndex = 0;
    ts->msResizing = 0;
    ts->oneDuration = 0;

    /* Wrap plugin functions */
    WRAP (ts, s, paintOutput, tilePaintOutput);
    WRAP (ts, s, preparePaintScreen, tilePreparePaintScreen);
    WRAP (ts, s, paintScreen, tilePaintScreen);
    WRAP (ts, s, donePaintScreen, tileDonePaintScreen);
    WRAP (ts, s, windowResizeNotify, tileResizeNotify);
    WRAP (ts, s, paintWindow, tilePaintWindow);

    return TRUE;
}

static void
tileFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    TILE_SCREEN (s);

    freeWindowPrivateIndex (s, ts->windowPrivateIndex);

    /* Restore the original function */
    UNWRAP (ts, s, paintOutput);
    UNWRAP (ts, s, preparePaintScreen);
    UNWRAP (ts, s, paintScreen);
    UNWRAP (ts, s, donePaintScreen);
    UNWRAP (ts, s, windowResizeNotify);
    UNWRAP (ts, s, paintWindow);

    free (ts);
}

/* this is resizeConstrainMinMax from resize.c,
   thanks to David Reveman/Nigel Cunningham */
static void
constrainMinMax (CompWindow *w,
		 int        width,
		 int        height,
		 int        *newWidth,
		 int        *newHeight)
{
    const XSizeHints *hints = &w->sizeHints;
    int              min_width = 0;
    int              min_height = 0;
    int              max_width = MAXSHORT;
    int              max_height = MAXSHORT;

    if ((hints->flags & PBaseSize) && (hints->flags & PMinSize))
    {
	min_width = hints->min_width;
	min_height = hints->min_height;
    }
    else if (hints->flags & PBaseSize)
    {
	min_width = hints->base_width;
	min_height = hints->base_height;
    }
    else if (hints->flags & PMinSize)
    {
	min_width = hints->min_width;
	min_height = hints->min_height;
    }

    if (hints->flags & PMaxSize)
    {
	max_width = hints->max_width;
	max_height = hints->max_height;
    }
#define CLAMP(v, min, max) ((v) <= (min) ? (min) : (v) >= (max) ? (max) : (v))

    /* clamp width and height to min and max values */
    width = CLAMP (width, min_width, max_width);
    height = CLAMP (height, min_height, max_height);

#undef CLAMP

    *newWidth = width;
    *newHeight = height;
}

/* Moves window to [x,y] and resizes to width x height
   if no animation or starts animation */
static Bool
placeWin (CompWindow *w,
	  int        x,
	  int        y,
	  int        width,
	  int        height)
{
    /* window existence check */
    if (!w)
	return FALSE;

    /* this checks if the window isnt smaller
       than minimum size it has defined */
    constrainMinMax (w, width, height, &width, &height);

    /* check if the window isnt already where its going to be */
    if (x == w->attrib.x && y == w->attrib.y &&
	width == w->attrib.width && height == w->attrib.height)
	return TRUE;

    TILE_WINDOW(w);

    /* set previous coordinates for animation */
    tw->prevCoords.x = w->attrib.x;
    tw->prevCoords.y = w->attrib.y;
    tw->prevCoords.width = w->attrib.width;
    tw->prevCoords.height = w->attrib.height;

    /* set future coordinates for animation */
    tw->newCoords.x = x;
    tw->newCoords.y = y;
    tw->newCoords.width = width;
    tw->newCoords.height = height;

    tw->alreadyResized = FALSE; /* window is not resized now */
    tw->needConfigure = TRUE;

    switch (tileGetAnimateType (w->screen->display))
    {
    case AnimateTypeNone:
	tileSetNewWindowSize (w);
	break;
    case AnimateTypeFilledOutline:
    case AnimateTypeSlide:
    case AnimateTypeZoom:
    case AnimateTypeDropFromTop:
	tileSetNewWindowSize (w);
	/* fall-through */
    case AnimateTypeFade:
	tw->animationType = Animating;
	break;
    default:
	break;
    }

    return TRUE;
}

static Bool
tileSetNewWindowSize (CompWindow *w)
{
    XWindowChanges xwc;
    unsigned int   mask = CWX | CWY | CWWidth | CWHeight;

    TILE_WINDOW (w);
    TILE_SCREEN (w->screen);

    xwc.x = tw->newCoords.x;
    xwc.y = tw->newCoords.y;
    xwc.width = tw->newCoords.width;
    xwc.height = tw->newCoords.height;

    if (ts->tileType == -1)
    {
	if (tw->savedValid)
	    maximizeWindow (w, tw->savedMaxState);
    }
    else
	maximizeWindow (w, 0);

    if (xwc.width == w->serverWidth)
	mask &= ~CWWidth;

    if (xwc.height == w->serverHeight)
	mask &= ~CWHeight;

    if (w->mapNum && (mask & (CWWidth | CWHeight)))
	sendSyncRequest (w);

    configureXWindow (w, mask, &xwc);
    tw->needConfigure = FALSE;

    return TRUE;
}

/* Heavily inspired by windowIs3D and isSDWindow,
   returns TRUE if the window is usable for tiling */
static Bool
isTileWindow (CompWindow *w)
{
    if (matchEval (tileGetExcludeMatch (w->screen->display), w))
	return FALSE;

    if (w->attrib.override_redirect)
	return FALSE;

    if (!((*w->screen->focusWindow) (w)))
	return FALSE;

    if (w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return FALSE;

    if (w->state & CompWindowStateSkipPagerMask)
	return FALSE;

    if (w->minimized || !w->placed)
	return FALSE;

    return TRUE;
}

/* save window coordinates to use for restore */
static void
saveCoords (CompWindow *w)
{
    TILE_WINDOW (w);

    if (tw->savedValid)
	return;

    tw->savedCoords.x = w->serverX;
    tw->savedCoords.y = w->serverY;
    tw->savedCoords.width = w->serverWidth;
    tw->savedCoords.height = w->serverHeight;

    tw->savedMaxState = w->state & MAXIMIZE_STATE;

    tw->savedValid = TRUE;
}

/* Applies tiling/restoring */
static Bool
applyTiling (CompScreen *s)
{
    CompWindow *w;
    int        count = 0;

    TILE_SCREEN (s);

    if (ts->grabIndex)
	return FALSE;

    for (w = s->windows; w; w = w->next)
    {
	if (isTileWindow (w))
	    count++;
    }

    ts->oneDuration = tileGetAnimationDuration (s->display) / MAX (count, 1);

    if (count > 1)
    {
	int               countX = 0, countY = 0;
	int               currentX = 0, currentY = 0;
	int               winWidth = 0, winHeight = 0;
	int               x = 0, y = 0;
	int               height = 0, occupancy = 0, delta = 0;
	Bool              first = TRUE;
	int               i = 0;
	XRectangle        workArea;
	CompWindowExtents border;

	memset (&border, 0, sizeof (CompWindowExtents));
	/* first get the largest border of the windows on this
	   screen - some of the windows in our list might be
	   maximized now and not be maximized later, so
	   their border information may be inaccurate */
	for (w = s->windows; w; w = w->next)
	{
	    if (w->input.left > border.left)
		border.left = w->input.left;
	    if (w->input.right > border.right)
		border.right = w->input.right;
	    if (w->input.top > border.top)
		border.top = w->input.top;
	    if (w->input.bottom > border.bottom)
		border.bottom = w->input.bottom;
	}

	workArea = s->workArea;

	switch (ts->tileType)
	{
	case TileToggleTypeTile:
	    countX = ceil (sqrt (count));
	    countY = ceil ((float)count / countX);
	    currentX = workArea.x;
	    currentY = workArea.y;
	    winWidth = workArea.width / countX;
	    winHeight = workArea.height / countY;
	    break;
	case TileToggleTypeLeft:
	    height = workArea.height / (count - 1);
	    occupancy = tileGetTileLeftOccupancy (s->display);
	    break;
	case TileToggleTypeTileVertically:
	    winWidth = workArea.width / count;
	    winHeight = workArea.height;
	    y = workArea.y;
	    break;
	case TileToggleTypeTileHorizontally:
	    winWidth = workArea.width;
	    winHeight = workArea.height / count;
	    x = workArea.x;
	    break;
	case TileToggleTypeCascade:
	    delta = tileGetTileDelta (s->display);
	    currentX = workArea.x;
	    currentY = workArea.y;
	    winHeight = workArea.height - delta * (count - 1);
	    winWidth = workArea.width - delta * (count - 1);
	    break;
	default:
	    break;
	}

	for (w = s->windows; w; w = w->next)
	{
	    if (!isTileWindow (w))
		continue;

	    TILE_WINDOW (w);

	    if (!tw->savedValid)
		saveCoords (w);

	    switch (ts->tileType)
	    {
	    case TileToggleTypeTile:
		placeWin(w,
			 currentX + border.left, currentY + border.top,
			 winWidth - (border.left + border.right),
			 winHeight - (border.top + border.bottom));
		tw->isTiled = TRUE;
		break;
	    case TileToggleTypeLeft:
		if (first)
		{
		    x = workArea.x;
		    y = workArea.y;
		    winWidth = workArea.width * occupancy / 100;
		    winHeight = workArea.height;
		    first = FALSE;
		}
		else
		{
		    x = workArea.x + (workArea.width * occupancy / 100);
		    y = workArea.y + (i * height);
		    winWidth = (workArea.width * (100 - occupancy) / 100);
		    winHeight = height;
		}

		placeWin(w,
			 x + border.left, y + border.top,
			 winWidth - (border.left + border.right),
			 winHeight - (border.top + border.bottom));
		tw->isTiled = TRUE;
		break;
	    case TileToggleTypeTileVertically:
		x = workArea.x + (winWidth * i);
		placeWin(w,
			 x + border.left, y + border.top,
			 winWidth - (border.left + border.right),
			 winHeight - (border.top + border.bottom));
		tw->isTiled = TRUE;
		break;
	    case TileToggleTypeTileHorizontally:
		y = workArea.y + (winHeight * i);
		placeWin (w, x + border.left, y + border.top,
			  winWidth - (border.left + border.right),
			  winHeight - (border.top + border.bottom));
		tw->isTiled = TRUE;
		break;
	    case TileToggleTypeCascade:
		placeWin (w,
			  currentX + border.left, currentY + border.top,
			  winWidth - (border.left + border.right),
			  winHeight - (border.top + border.bottom));
		tw->isTiled = TRUE;
		break;
	    default:
		break;
	    }

	    if (ts->tileType == -1 && tw->isTiled)
		{
		    placeWin (w,
			      tw->savedCoords.x, tw->savedCoords.y,
			      tw->savedCoords.width, tw->savedCoords.height);
		    tw->savedValid = FALSE;
		    tw->isTiled = FALSE;
		}

	    i++;
	    tw->animationNum = i;

	    switch (ts->tileType)
	    {
	    case TileToggleTypeTile:
		if (!(i % countX))
		{
		    currentX = workArea.x;
		    currentY += winHeight;
		}
		else
		    currentX += winWidth;
		break;
	    case TileToggleTypeCascade:
		currentX += delta;
		currentY += delta;
	    default:
		break;
	    }
	}

	if (!ts->grabIndex)
	    ts->grabIndex = pushScreenGrab (s, s->invisibleCursor, "tile");

	ts->msResizing = 0;
    }

    return TRUE;
}

static Bool
tileTile (CompDisplay     *d,
	  CompAction      *action,
	  CompActionState state,
	  CompOption      *option,
	  int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	TILE_SCREEN (s);

	ts->tileType = TileToggleTypeTile;
	applyTiling (s);
    }

    return FALSE;
}

static Bool
tileVertically (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	TILE_SCREEN (s);

	ts->tileType = TileToggleTypeTileVertically;
	applyTiling (s);
    }

    return FALSE;
}

static Bool
tileHorizontally (CompDisplay     *d,
		  CompAction      *action,
		  CompActionState state,
		  CompOption      *option,
		  int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	TILE_SCREEN (s);

	ts->tileType = TileToggleTypeTileHorizontally;
	applyTiling (s);
    }

    return FALSE;
}

static Bool
tileCascade (CompDisplay     *d,
	     CompAction      *action,
	     CompActionState state,
	     CompOption      *option,
	     int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	TILE_SCREEN (s);

	ts->tileType = TileToggleTypeCascade;
	applyTiling (s);
    }

    return FALSE;
}

static Bool
tileRestore (CompDisplay     *d,
	     CompAction      *action,
	     CompActionState state,
	     CompOption      *option,
	     int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	TILE_SCREEN (s);

	ts->tileType = -1;
	applyTiling (s);
    }

    return FALSE;
}

static Bool
tileToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	CompWindow *w;

	TILE_SCREEN (s);

	for (w = s->windows; w; w = w->next)
	{
	    TILE_WINDOW (w);
	    if (tw->isTiled)
		break;
	}

	if (w)
	{
	    ts->tileType = -1;
	    applyTiling (s);
	}
	else
	{
	    ts->tileType = tileGetTileToggleType (d);
	    applyTiling (s);
	}
    }

    return FALSE;
}

static Bool
tileInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    TileDisplay *td;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    td = malloc (sizeof (TileDisplay));
    if (!td)
	return FALSE;

    /* Allocate a private index */
    td->screenPrivateIndex = allocateScreenPrivateIndex (d);

    /* Check if its valid */
    if (td->screenPrivateIndex < 0)
    {
	free (td);
	return FALSE;
    }

    tileSetTileVerticallyKeyInitiate (d, tileVertically);
    tileSetTileHorizontallyKeyInitiate (d, tileHorizontally);
    tileSetTileTileKeyInitiate (d, tileTile);
    tileSetTileCascadeKeyInitiate (d, tileCascade);
    tileSetTileRestoreKeyInitiate (d, tileRestore);
    tileSetTileToggleKeyInitiate (d, tileToggle);

    /* Record the display */
    d->base.privates[displayPrivateIndex].ptr = td;

    return TRUE;
}

static void
tileFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    TILE_DISPLAY (d);

    /* Free the private index */
    freeScreenPrivateIndex (d, td->screenPrivateIndex);

    free (td);
}

static Bool
tileInitWindow (CompPlugin *p,
		CompWindow *w)
{
    TileWindow *tw;

    TILE_SCREEN (w->screen);

    tw = malloc (sizeof (TileWindow));
    if (!tw)
	return FALSE;

    memset (&tw->newCoords, 0, sizeof (XRectangle));
    memset (&tw->prevCoords, 0, sizeof (XRectangle));
    memset (&tw->savedCoords, 0, sizeof (XRectangle));

    tw->savedValid = FALSE;
    tw->animationType = NoAnimation;
    tw->savedMaxState = 0;
    tw->isTiled = FALSE;
    tw->needConfigure = FALSE;

    /* random color, from group.c thanks to the author for doing it */
    tw->outlineColor[0] = rand() % 0xFFFF;
    tw->outlineColor[1] = rand() % 0xFFFF;
    tw->outlineColor[2] = rand() % 0xFFFF;

    w->base.privates[ts->windowPrivateIndex].ptr = tw;

    return TRUE;
}

static void
tileFiniWindow (CompPlugin *p,
		CompWindow *w)
{
    TILE_WINDOW (w);

    free (tw);
}

static CompBool
tileInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) tileInitDisplay,
	(InitPluginObjectProc) tileInitScreen,
	(InitPluginObjectProc) tileInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
tileFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) tileFiniDisplay,
	(FiniPluginObjectProc) tileFiniScreen,
	(FiniPluginObjectProc) tileFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
tileInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
tileFini (CompPlugin * p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginVTable tileVTable = {
    "tile",
    0,
    tileInit,
    tileFini,
    tileInitObject,
    tileFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &tileVTable;
}
