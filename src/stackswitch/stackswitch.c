/*
 *
 * Compiz stackswitch switcher plugin
 *
 * stackswitch.c
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Based on scale.c and switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <compiz-core.h>
#include <compiz-text.h>
#include "stackswitch_options.h"

typedef enum {
    StackswitchStateNone = 0,
    StackswitchStateOut,
    StackswitchStateSwitching,
    StackswitchStateIn
} StackswitchState;

typedef enum {
    StackswitchTypeNormal = 0,
    StackswitchTypeGroup,
    StackswitchTypeAll
} StackswitchType;

static int StackswitchDisplayPrivateIndex;

typedef struct _StackswitchSlot {
    int   x, y;            /* thumb center coordinates */
    float scale;           /* size scale (fit to maximal thumb size) */
} StackswitchSlot;

typedef struct _StackswitchDrawSlot {
    CompWindow      *w;
    StackswitchSlot **slot;
} StackswitchDrawSlot;

typedef struct _StackswitchDisplay {
    int		    screenPrivateIndex;
    HandleEventProc handleEvent;
    TextFunc        *textFunc;
} StackswitchDisplay;

typedef struct _StackswitchScreen {
    int windowPrivateIndex;

    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc        paintOutput;
    PaintWindowProc        paintWindow;
    DamageWindowRectProc   damageWindowRect;

    int  grabIndex;

    StackswitchState state;
    StackswitchType  type;
    Bool              moreAdjust;
    Bool              rotateAdjust;

    Bool paintingSwitcher;

    GLfloat rVelocity;
    GLfloat rotation;

    /* only used for sorting */
    CompWindow          **windows;
    StackswitchDrawSlot *drawSlots;
    int                 windowsSize;
    int                 nWindows;

    Window clientLeader;

    CompWindow *selectedWindow;

    /* text display support */
    CompTextData *textData;

    CompMatch match;
    CompMatch *currentMatch;
} StackswitchScreen;

typedef struct _StackswitchWindow {

    StackswitchSlot *slot;

    GLfloat xVelocity;
    GLfloat yVelocity;
    GLfloat scaleVelocity;
    GLfloat rotVelocity;

    GLfloat tx;
    GLfloat ty;
    GLfloat scale;
    GLfloat rotation;
    Bool    adjust;
} StackswitchWindow;

#define STACKSWITCH_DISPLAY(d) PLUGIN_DISPLAY(d, Stackswitch, s)
#define STACKSWITCH_SCREEN(sc) PLUGIN_SCREEN(sc, Stackswitch, s)
#define STACKSWITCH_WINDOW(w) PLUGIN_WINDOW(w, Stackswitch, s)

static void
stackswitchActivateEvent (CompScreen *s,
			  Bool       activating)
{
    CompOption o[2];

    o[0].type = CompOptionTypeInt;
    o[0].name = "root";
    o[0].value.i = s->root;

    o[1].type = CompOptionTypeBool;
    o[1].name = "active";
    o[1].value.b = activating;

    (*s->display->handleCompizEvent) (s->display, "stackswitch", "activate", o, 2);
}

static Bool
isStackswitchWin (CompWindow *w)
{
    STACKSWITCH_SCREEN (w->screen);

    if (w->destroyed)
	return FALSE;

    if (w->attrib.override_redirect)
	return FALSE;

    if (w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return FALSE;

    if (!w->mapNum || w->attrib.map_state != IsViewable)
    {
	if (stackswitchGetMinimized (w->screen))
	{
	    if (!w->minimized && !w->inShowDesktopMode && !w->shaded)
		return FALSE;
	}
	else
	    return FALSE;
    }

    if (ss->type == StackswitchTypeNormal)
    {
	if (!w->mapNum || w->attrib.map_state != IsViewable)
	{
	    if (w->serverX + w->width  <= 0    ||
		w->serverY + w->height <= 0    ||
		w->serverX >= w->screen->width ||
		w->serverY >= w->screen->height)
		return FALSE;
	}
	else
	{
	    if (!(*w->screen->focusWindow) (w))
		return FALSE;
	}
    }
    else if (ss->type == StackswitchTypeGroup &&
	     ss->clientLeader != w->clientLeader &&
	     ss->clientLeader != w->id)
    {
	return FALSE;
    }

    if (w->state & CompWindowStateSkipTaskbarMask)
	return FALSE;

    if (!matchEval (ss->currentMatch, w))
	return FALSE;

    return TRUE;
}

static void
stackswitchFreeWindowTitle (CompScreen *s)
{
    STACKSWITCH_SCREEN (s);
    STACKSWITCH_DISPLAY (s->display);

    if (!ss->textData)
	return;

    (sd->textFunc->finiTextData) (s, ss->textData);
    ss->textData = NULL;
}

static void
stackswitchRenderWindowTitle (CompScreen *s)
{
    CompTextAttrib tA;
    int            ox1, ox2, oy1, oy2;
    Bool           showViewport;

    STACKSWITCH_SCREEN (s);
    STACKSWITCH_DISPLAY (s->display);

    stackswitchFreeWindowTitle (s);

    if (!sd->textFunc)
	return;

    if (!stackswitchGetWindowTitle (s))
	return;

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    /* 75% of the output device as maximum width */
    tA.maxWidth = (ox2 - ox1) * 3 / 4;
    tA.maxHeight = 100;

    tA.family = stackswitchGetTitleFontFamily (s);
    tA.size = stackswitchGetTitleFontSize (s);
    tA.color[0] = stackswitchGetTitleFontColorRed (s);
    tA.color[1] = stackswitchGetTitleFontColorGreen (s);
    tA.color[2] = stackswitchGetTitleFontColorBlue (s);
    tA.color[3] = stackswitchGetTitleFontColorAlpha (s);

    tA.flags = CompTextFlagWithBackground | CompTextFlagEllipsized;
    if (stackswitchGetTitleFontBold (s))
	tA.flags |= CompTextFlagStyleBold;

    tA.bgHMargin = 15;
    tA.bgVMargin = 15;
    tA.bgColor[0] = stackswitchGetTitleBackColorRed (s);
    tA.bgColor[1] = stackswitchGetTitleBackColorGreen (s);
    tA.bgColor[2] = stackswitchGetTitleBackColorBlue (s);
    tA.bgColor[3] = stackswitchGetTitleBackColorAlpha (s);

    showViewport = ss->type == StackswitchTypeAll;
    ss->textData = (sd->textFunc->renderWindowTitle) (s,
						      (ss->selectedWindow ?
						       ss->selectedWindow->id :
						       None),
						      showViewport, &tA);
}

static void
stackswitchDrawWindowTitle (CompScreen    *s,
			    CompTransform *transform,
			    CompWindow    *w)
{
    GLboolean     wasBlend;
    GLint         oldBlendSrc, oldBlendDst;
    CompTransform wTransform = *transform, mvp, pm;
    float         x, y, tx, ix, width, height;
    int           ox1, ox2, oy1, oy2, i;
    CompMatrix    *m;
    CompVector    v;
    CompIcon      *icon;

    STACKSWITCH_SCREEN (s);
    STACKSWITCH_WINDOW (w);

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    width = ss->textData->width;
    height = ss->textData->height;

    x = ox1 + ((ox2 - ox1) / 2);
    tx = x - (ss->textData->width / 2);

    switch (stackswitchGetTitleTextPlacement (s))
    {
	case TitleTextPlacementOnThumbnail:
	    v.x = w->attrib.x + (w->attrib.width / 2.0);
	    v.y = w->attrib.y + (w->attrib.height / 2.0);
	    v.z = 0.0;
	    v.w = 1.0;

	    matrixScale (&wTransform, 1.0, 1.0, 1.0 / s->height);
	    matrixTranslate (&wTransform, sw->tx, sw->ty, 0.0f);
	    matrixRotate (&wTransform, -ss->rotation, 1.0, 0.0, 0.0);
	    matrixScale (&wTransform, sw->scale, sw->scale, 1.0);
	    matrixTranslate (&wTransform, +w->input.left, 0.0 -(w->attrib.height + w->input.bottom), 0.0f);
	    matrixTranslate (&wTransform, -w->attrib.x, -w->attrib.y, 0.0f);

	    for (i=0; i < 16; i++)
		pm.m[i] = s->projection[i];
	    matrixMultiply (&mvp, &pm, &wTransform);
	    matrixMultiplyVector (&v, &v, &mvp);
	    matrixVectorDiv (&v);

	    x = (v.x + 1.0) * (ox2 - ox1) * 0.5;
	    y = (v.y - 1.0) * (oy2 - oy1) * -0.5;

	    x += ox1;
	    y += oy1;

	    tx = MAX (ox1, x - (width / 2.0));
	    if (tx + width > ox2)
		tx = ox2 - width;
	    break;
	case TitleTextPlacementCenteredOnScreen:
	    y = oy1 + ((oy2 - oy1) / 2) + (height / 2);
	    break;
	case TitleTextPlacementAbove:
	case TitleTextPlacementBelow:
	    {
		XRectangle workArea;
		getWorkareaForOutput (s, s->currentOutputDev, &workArea);

		if (stackswitchGetTitleTextPlacement (s) ==
		    TitleTextPlacementAbove)
		    y = oy1 + workArea.y + height;
		else
		    y = oy1 + workArea.y + workArea.height - 96;
	    }
	    break;
	default:
	    return;
	    break;
    }

    tx = floor (tx);
    y  = floor (y);

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    wasBlend = glIsEnabled (GL_BLEND);
    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (1.0, 1.0, 1.0, 1.0);

    icon = getWindowIcon (w, 96, 96);
    if (!icon)
	icon = w->screen->defaultIcon;

    if (icon && (icon->texture.name || iconToTexture (w->screen, icon)))
    {
	int                 off;

	ix = x - (icon->width / 2.0);
	ix  = floor (ix);

	enableTexture (s, &icon->texture,COMP_TEXTURE_FILTER_GOOD);

	m = &icon->texture.matrix;

	glColor4f (0.0, 0.0, 0.0, 0.1);

	for (off = 0; off < 6; off++)
	{
	    glBegin (GL_QUADS);

	    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
	    glVertex2f (ix - off, y - off);
	    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, icon->height));
	    glVertex2f (ix - off, y + icon->height + off);
	    glTexCoord2f (COMP_TEX_COORD_X (m, icon->width), COMP_TEX_COORD_Y (m, icon->height));
	    glVertex2f (ix + icon->width + off, y + icon->height + off);
	    glTexCoord2f (COMP_TEX_COORD_X (m, icon->width), COMP_TEX_COORD_Y (m, 0));
	    glVertex2f (ix + icon->width + off, y - off);

	    glEnd ();
	}

	glColor4f (1.0, 1.0, 1.0, 1.0);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
	glVertex2f (ix, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, icon->height));
	glVertex2f (ix, y + icon->height);
	glTexCoord2f (COMP_TEX_COORD_X (m, icon->width), COMP_TEX_COORD_Y (m, icon->height));
	glVertex2f (ix + icon->width, y + icon->height);
	glTexCoord2f (COMP_TEX_COORD_X (m, icon->width), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (ix + icon->width, y);

	glEnd ();

	disableTexture (s, &icon->texture);
    }

    enableTexture (s, ss->textData->texture, COMP_TEXTURE_FILTER_GOOD);

    m = &ss->textData->texture->matrix;

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
    glVertex2f (tx, y - height);
    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
    glVertex2f (tx, y);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
    glVertex2f (tx + width, y);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
    glVertex2f (tx + width, y - height);

    glEnd ();

    disableTexture (s, ss->textData->texture);
    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

static Bool
stackswitchPaintWindow (CompWindow              *w,
			const WindowPaintAttrib *attrib,
			const CompTransform	*transform,
			Region		        region,
			unsigned int		mask)
{
    CompScreen *s = w->screen;
    Bool       status;

    STACKSWITCH_SCREEN (s);

    if (ss->state != StackswitchStateNone)
    {
	WindowPaintAttrib sAttrib = *attrib;
	Bool		  scaled = FALSE;
	float             rotation;

	STACKSWITCH_WINDOW (w);

	if (w->mapNum)
	{
	    if (!w->texture->pixmap && !w->bindFailed)
		bindWindow (w);
	}

	if (sw->adjust || sw->slot)
	{
	    scaled = (sw->adjust && ss->state != StackswitchStateSwitching) ||
		     (sw->slot && ss->paintingSwitcher);
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
	}
	else if (ss->state != StackswitchStateIn)
	{
	    if (stackswitchGetDarkenBack (s))
	    {
		/* modify brightness of the other windows */
		sAttrib.brightness = sAttrib.brightness / 2;
	    }
	}

	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, &sAttrib, transform, region, mask);
	WRAP (ss, s, paintWindow, stackswitchPaintWindow);

	if (stackswitchGetInactiveRotate(s))
	    rotation = MIN (sw->rotation, ss->rotation);
	else
	    rotation = ss->rotation;

	if (scaled && w->texture->pixmap)
	{
	    FragmentAttrib fragment;
	    CompTransform  wTransform = *transform;

	    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
		return FALSE;

	    initFragmentAttrib (&fragment, &w->lastPaint);

	    if (sw->slot)
	    {
		if (w != ss->selectedWindow)
		    fragment.opacity = (float)fragment.opacity *
			               stackswitchGetInactiveOpacity (s) / 100;
	    }

	    if (w->alpha || fragment.opacity != OPAQUE)
		mask |= PAINT_WINDOW_TRANSLUCENT_MASK;


	    matrixScale (&wTransform, 1.0, 1.0, 1.0 / s->height);
	    matrixTranslate (&wTransform, sw->tx, sw->ty, 0.0f);

	    matrixRotate (&wTransform, -rotation, 1.0, 0.0, 0.0);
	    matrixScale (&wTransform, sw->scale, sw->scale, 1.0);

	    matrixTranslate (&wTransform, +w->input.left, 0.0 -(w->attrib.height + w->input.bottom), 0.0f);
	    matrixTranslate (&wTransform, -w->attrib.x, -w->attrib.y, 0.0f);
	    glPushMatrix ();
	    glLoadMatrixf (wTransform.m);

	    (*s->drawWindow) (w, &wTransform, &fragment, region,
			      mask | PAINT_WINDOW_TRANSFORMED_MASK);

	    glPopMatrix ();
	}

	if (scaled && !w->texture->pixmap)
	{
	    CompIcon *icon;

	    icon = getWindowIcon (w, 96, 96);
	    if (!icon)
		icon = w->screen->defaultIcon;

	    if (icon && (icon->texture.name || iconToTexture (w->screen, icon)))
	    {
		REGION              iconReg;
		CompMatrix          matrix;
		float               scale;

		scale = MIN (w->attrib.width / icon->width,
			     w->attrib.height / icon->height);
		scale *= sw->scale;

		mask |= PAINT_WINDOW_BLEND_MASK;

		/* if we paint the icon for a minimized window, we need
		   to force the usage of a good texture filter */
		if (!w->texture->pixmap)
		    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		iconReg.rects    = &iconReg.extents;
		iconReg.numRects = 1;

		iconReg.extents.x1 = 0;
		iconReg.extents.y1 = 0;
		iconReg.extents.x2 = icon->width;
		iconReg.extents.y2 = icon->height;

		matrix = icon->texture.matrix;

		w->vCount = w->indexCount = 0;
		(*w->screen->addWindowGeometry) (w, &matrix, 1,
						 &iconReg, &infiniteRegion);

		if (w->vCount)
		{
		    FragmentAttrib fragment;
		    CompTransform  wTransform = *transform;

		    if (!w->texture->pixmap)
			sAttrib.opacity = w->paint.opacity;

		    initFragmentAttrib (&fragment, &sAttrib);

		    matrixScale (&wTransform, 1.0, 1.0, 1.0 / s->height);
		    matrixTranslate (&wTransform, sw->tx, sw->ty, 0.0f);

		    matrixRotate (&wTransform, -rotation, 1.0, 0.0, 0.0);
		    matrixScale (&wTransform, scale, scale, 1.0);

		    matrixTranslate (&wTransform, 0.0, -icon->height, 0.0f);

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.m);

		    (*w->screen->drawWindowTexture) (w, &icon->texture,
						     &fragment, mask);

		    glPopMatrix ();
		}
	    }
	}
    }
    else
    {
	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ss, s, paintWindow, stackswitchPaintWindow);
    }

    return status;
}

static int
compareWindows (const void *elem1,
		const void *elem2)
{
    CompWindow *w1 = *((CompWindow **) elem1);
    CompWindow *w2 = *((CompWindow **) elem2);

    if (w1->mapNum && !w2->mapNum)
	return -1;

    if (w2->mapNum && !w1->mapNum)
	return 1;

    return w2->activeNum - w1->activeNum;
}

static int
compareStackswitchWindowDepth (const void *elem1,
			       const void *elem2)
{
    StackswitchSlot *a1   = *(((StackswitchDrawSlot *) elem1)->slot);
    StackswitchSlot *a2   = *(((StackswitchDrawSlot *) elem2)->slot);

    if (a1->y < a2->y)
	return -1;
    else if (a1->y > a2->y)
	return 1;
    else
    {
	CompWindow *w1   = (((StackswitchDrawSlot *) elem1)->w);
	CompWindow *w2   = (((StackswitchDrawSlot *) elem2)->w);
	STACKSWITCH_SCREEN (w1->screen);
	if (w1 == ss->selectedWindow)
	    return 1;
	else if (w2 == ss->selectedWindow)
	    return -1;
	else
	    return 0;
    }
}

static Bool
layoutThumbs (CompScreen *s)
{
    CompWindow *w;
    int        index;
    int        ww, wh;
    float      xScale, yScale;
    int        ox1, ox2, oy1, oy2;
    float      swi = 0.0, oh, rh, ow;
    int        cols, rows, col = 0, row = 0, r, c;
    int        cindex, ci, gap, hasActive = 0;
    Bool       exit;

    STACKSWITCH_SCREEN (s);

    if ((ss->state == StackswitchStateNone) || (ss->state == StackswitchStateIn))
	return FALSE;

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);
    ow = (float)(ox2 - ox1) * 0.9;

    for (index = 0; index < ss->nWindows; index++)
    {
	w = ss->windows[index];

	ww = w->attrib.width  + w->input.left + w->input.right;
	wh = w->attrib.height + w->input.top  + w->input.bottom;

	swi += ((float)ww / (float)wh) * (ow / (float)(oy2 - oy1));
    }

    cols = ceil (sqrtf (swi));

    swi = 0.0;
    for (index = 0; index < ss->nWindows; index++)
    {
	w = ss->windows[index];

	ww = w->attrib.width  + w->input.left + w->input.right;
	wh = w->attrib.height + w->input.top  + w->input.bottom;

	swi += (float)ww / (float)wh;

	if (swi > cols)
	{
	    row++;
	    swi = (float)ww / (float)wh;
	    col = 0;
	}

	col++;
    }
    rows = row + 1;

    oh = ow / cols;
    oh *= (float)(oy2 - oy1) / (float)(oy2 - oy1);

    rh = ((float)(oy2 - oy1) * 0.8) / rows;

    for (index = 0; index < ss->nWindows; index++)
    {
	w = ss->windows[index];

	STACKSWITCH_WINDOW (w);

	if (!sw->slot)
	    sw->slot = malloc (sizeof (StackswitchSlot));

	if (!sw->slot)
	    return FALSE;

	ss->drawSlots[index].w    = w;
	ss->drawSlots[index].slot = &sw->slot;
    }

    index = 0;

    for (r = 0; r < rows && index < ss->nWindows; r++)
    {
	c = 0;
	swi = 0.0;
	cindex = index;
	exit = FALSE;
	while (index < ss->nWindows && !exit)
	{
	    w = ss->windows[index];

	    STACKSWITCH_WINDOW (w);
	    sw->slot->x = ox1 + swi;
	    sw->slot->y = oy2 - (rh * r) - ((oy2 - oy1) * 0.1);


	    ww = w->attrib.width  + w->input.left + w->input.right;
	    wh = w->attrib.height + w->input.top  + w->input.bottom;

	    if (ww > ow)
		xScale = ow / (float) ww;
	    else
		xScale = 1.0f;

	    if (wh > oh)
		yScale = oh / (float) wh;
	    else
		yScale = 1.0f;

	    sw->slot->scale = MIN (xScale, yScale);

	    if (swi + (ww * sw->slot->scale) > ow && cindex != index)
	    {
		exit = TRUE;
		continue;
	    }

	    if (w == ss->selectedWindow)
		hasActive = 1;
	    swi += ww * sw->slot->scale;

	    c++;
	    index++;
	}

	gap = ox2 - ox1 - swi;
	gap /= c + 1;

	index = cindex;
	ci = 1;
	while (ci <= c)
	{
	    w = ss->windows[index];

	    STACKSWITCH_WINDOW (w);
	    sw->slot->x += ci * gap;

	    if (hasActive == 0)
		sw->slot->y += sqrt(2 * oh * oh) - rh;

	    ci++;
	    index++;
	}

	if (hasActive == 1)
	    hasActive++;
    }

    /* sort the draw list so that the windows with the
       lowest Y value (the windows being farest away)
       are drawn first */
    qsort (ss->drawSlots, ss->nWindows, sizeof (StackswitchDrawSlot),
	   compareStackswitchWindowDepth);

    return TRUE;
}

static void
stackswitchAddWindowToList (CompScreen *s,
			    CompWindow *w)
{
    STACKSWITCH_SCREEN (s);

    if (ss->windowsSize <= ss->nWindows)
    {
	ss->windows = realloc (ss->windows,
			       sizeof (CompWindow *) * (ss->nWindows + 32));
	if (!ss->windows)
	    return;

	ss->drawSlots = realloc (ss->drawSlots,
				 sizeof (StackswitchDrawSlot) * (ss->nWindows + 32));

	if (!ss->drawSlots)
	    return;

	ss->windowsSize = ss->nWindows + 32;
    }

    ss->windows[ss->nWindows++] = w;
}

static Bool
stackswitchUpdateWindowList (CompScreen *s)
{
    STACKSWITCH_SCREEN (s);

    qsort (ss->windows, ss->nWindows, sizeof (CompWindow *), compareWindows);

    return layoutThumbs (s);
}

static Bool
stackswitchCreateWindowList (CompScreen *s)
{
    CompWindow *w;

    STACKSWITCH_SCREEN (s);

    ss->nWindows = 0;

    for (w = s->windows; w; w = w->next)
    {
	if (isStackswitchWin (w))
	{
	    STACKSWITCH_WINDOW (w);

	    stackswitchAddWindowToList (s, w);
	    sw->adjust = TRUE;
	}
    }

    return stackswitchUpdateWindowList (s);
}

static void
switchToWindow (CompScreen *s,
		Bool	   toNext)
{
    CompWindow *w;
    int	       cur;

    STACKSWITCH_SCREEN (s);

    if (!ss->grabIndex)
	return;

    for (cur = 0; cur < ss->nWindows; cur++)
    {
	if (ss->windows[cur] == ss->selectedWindow)
	    break;
    }

    if (cur == ss->nWindows)
	return;

    if (toNext)
	w = ss->windows[(cur + 1) % ss->nWindows];
    else
	w = ss->windows[(cur + ss->nWindows - 1) % ss->nWindows];

    if (w)
    {
	CompWindow *old = ss->selectedWindow;

	ss->selectedWindow = w;
	if (old != w)
	{

	    ss->rotateAdjust = TRUE;
	    ss->moreAdjust = TRUE;
	    for (w = s->windows; w; w = w->next)
	    {
		STACKSWITCH_WINDOW (w);
		sw->adjust = TRUE;
	    }

	    damageScreen (s);
	    stackswitchRenderWindowTitle (s);
	}
    }
}

static int
stackswitchCountWindows (CompScreen *s)
{
    CompWindow *w;
    int	       count = 0;

    for (w = s->windows; w; w = w->next)
    {
	if (isStackswitchWin (w))
	    count++;
    }

    return count;
}

static int
adjustStackswitchRotation (CompScreen *s,
			   float      chunk)
{
    float dx, adjust, amount, rot;

    STACKSWITCH_SCREEN(s);

    if (ss->state != StackswitchStateNone && ss->state != StackswitchStateIn)
	rot = stackswitchGetTilt (s);
    else
	rot = 0.0;

    dx = rot - ss->rotation;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    ss->rVelocity = (amount * ss->rVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (ss->rVelocity) < 0.2f)
    {
	ss->rVelocity = 0.0f;
	ss->rotation = rot;
	return FALSE;
    }

    ss->rotation += ss->rVelocity * chunk;
    return TRUE;
}

static int
adjustStackswitchVelocity (CompWindow *w)
{
    float dx, dy, ds, dr, adjust, amount;
    float x1, y1, scale, rot;

    STACKSWITCH_WINDOW (w);
    STACKSWITCH_SCREEN (w->screen);

    if (sw->slot)
    {
	scale = sw->slot->scale;
	x1 = sw->slot->x;
	y1 = sw->slot->y;
    }
    else
    {
	scale = 1.0f;
	x1 = w->attrib.x - w->input.left;
	y1 = w->attrib.y + w->attrib.height + w->input.bottom;
    }

    if (w == ss->selectedWindow)
	rot = ss->rotation;
    else
	rot = 0.0;

    dx = x1 - sw->tx;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    sw->xVelocity = (amount * sw->xVelocity + adjust) / (amount + 1.0f);

    dy = y1 - sw->ty;

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    sw->yVelocity = (amount * sw->yVelocity + adjust) / (amount + 1.0f);

    ds = scale - sw->scale;
    adjust = ds * 0.1f;
    amount = fabs (ds) * 7.0f;
    if (amount < 0.01f)
	amount = 0.01f;
    else if (amount > 0.15f)
	amount = 0.15f;

    sw->scaleVelocity = (amount * sw->scaleVelocity + adjust) /
	(amount + 1.0f);

    dr = rot - sw->rotation;
    adjust = dr * 0.15f;
    amount = fabs (dr) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    sw->rotVelocity = (amount * sw->rotVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (sw->xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (sw->yVelocity) < 0.2f &&
	fabs (ds) < 0.001f && fabs (sw->scaleVelocity) < 0.002f &&
        fabs (dr) < 0.1f && fabs (sw->rotVelocity) < 0.2f)
    {
	sw->xVelocity = sw->yVelocity = sw->scaleVelocity = 0.0f;
	sw->tx = x1;
	sw->ty = y1;
	sw->rotation = rot;
	sw->scale = scale;

	return 0;
    }

    return 1;
}

static Bool
stackswitchPaintOutput (CompScreen		*s,
			const ScreenPaintAttrib *sAttrib,
			const CompTransform	*transform,
			Region		        region,
			CompOutput		*output,
			unsigned int		mask)
{
    Bool status;
    CompTransform sTransform = *transform;

    STACKSWITCH_SCREEN (s);

    if (ss->state != StackswitchStateNone || ss->rotation != 0.0)
    {
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	mask |= PAINT_SCREEN_TRANSFORMED_MASK;
	mask |= PAINT_SCREEN_CLEAR_MASK;
	matrixTranslate (&sTransform, 0.0, -0.5, -DEFAULT_Z_CAMERA);
	matrixRotate (&sTransform, -ss->rotation, 1.0, 0.0, 0.0);
	matrixTranslate (&sTransform, 0.0, 0.5, DEFAULT_Z_CAMERA);
    }

    UNWRAP (ss, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, &sTransform, region, output, mask);
    WRAP (ss, s, paintOutput, stackswitchPaintOutput);

    if (ss->state != StackswitchStateNone && (output->id == ~0 ||
	s->outputDev[s->currentOutputDev].id == output->id))
    {
	int           i;
	CompWindow    *aw = NULL;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	ss->paintingSwitcher = TRUE;

	for (i = 0; i < ss->nWindows; i++)
	{
	    if (ss->drawSlots[i].slot && *(ss->drawSlots[i].slot))
	    {
		CompWindow *w = ss->drawSlots[i].w;
		if (w == ss->selectedWindow)
		    aw = w;

		(*s->paintWindow) (w, &w->paint, &sTransform,
				   &infiniteRegion, 0);
	    }
	}


	CompTransform tTransform = *transform;
	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &tTransform);
	glLoadMatrixf (tTransform.m);

	if (ss->textData && (ss->state != StackswitchStateIn) && aw)
	    stackswitchDrawWindowTitle (s, &sTransform, aw);

	ss->paintingSwitcher = FALSE;

	glPopMatrix ();
    }

    return status;
}

static void
stackswitchPreparePaintScreen (CompScreen *s,
			       int	  msSinceLastPaint)
{
    STACKSWITCH_SCREEN (s);

    if (ss->state != StackswitchStateNone && (ss->moreAdjust || ss->rotateAdjust))
    {
	CompWindow *w;
	int        steps;
	float      amount, chunk;

	amount = msSinceLastPaint * 0.05f * stackswitchGetSpeed (s);
	steps  = amount / (0.5f * stackswitchGetTimestep (s));

	if (!steps)
	    steps = 1;
	chunk  = amount / (float) steps;

	layoutThumbs (s);
	while (steps--)
	{
	    ss->rotateAdjust = adjustStackswitchRotation (s, chunk);
	    ss->moreAdjust = FALSE;

	    for (w = s->windows; w; w = w->next)
	    {
		STACKSWITCH_WINDOW (w);

		if (sw->adjust)
		{
		    sw->adjust = adjustStackswitchVelocity (w);

		    ss->moreAdjust |= sw->adjust;

		    sw->tx       += sw->xVelocity * chunk;
		    sw->ty       += sw->yVelocity * chunk;
		    sw->scale    += sw->scaleVelocity * chunk;
		    sw->rotation += sw->rotVelocity * chunk;
		}
		else if (sw->slot)
		{
		    sw->scale    = sw->slot->scale;
		    sw->tx       = sw->slot->x;
		    sw->ty       = sw->slot->y;
		    if (w == ss->selectedWindow)
			sw->rotation = ss->rotation;
		    else
			sw->rotation = 0.0;
		}
	    }

	    if (!ss->moreAdjust && !ss->rotateAdjust)
		break;
	}
    }

    UNWRAP (ss, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ss, s, preparePaintScreen, stackswitchPreparePaintScreen);
}

static void
stackswitchDonePaintScreen (CompScreen *s)
{
    STACKSWITCH_SCREEN (s);

    if (ss->state != StackswitchStateNone)
    {
	if (ss->moreAdjust)
	{
	    damageScreen (s);
	}
	else
	{
	    if (ss->rotateAdjust)
		damageScreen (s);

	    if (ss->state == StackswitchStateIn)
	    {
		ss->state = StackswitchStateNone;
		stackswitchActivateEvent (s, FALSE);
	    }
	    else if (ss->state == StackswitchStateOut)
		ss->state = StackswitchStateSwitching;
	}
    }

    UNWRAP (ss, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ss, s, donePaintScreen, stackswitchDonePaintScreen);
}

static Bool
stackswitchTerminate (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int	      nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    for (s = d->screens; s; s = s->next)
    {
	STACKSWITCH_SCREEN (s);

	if (xid && s->root != xid)
	    continue;

	if (ss->grabIndex)
	{
	    removeScreenGrab (s, ss->grabIndex, 0);
	    ss->grabIndex = 0;
	}

	if (ss->state != StackswitchStateNone)
	{
	    CompWindow *w;

	    for (w = s->windows; w; w = w->next)
	    {
		STACKSWITCH_WINDOW (w);

		if (sw->slot)
		{
		    free (sw->slot);
		    sw->slot = NULL;

		    sw->adjust = TRUE;
		}
	    }
	    ss->moreAdjust = TRUE;
	    ss->state = StackswitchStateIn;
	    damageScreen (s);

	    if (!(state & CompActionStateCancel) && ss->selectedWindow &&
		    !ss->selectedWindow->destroyed)
	    {
		sendWindowActivationRequest (s, ss->selectedWindow->id);
	    }
	}
    }

    if (action)
	action->state &= ~(CompActionStateTermKey |
			   CompActionStateTermButton |
			   CompActionStateTermEdge);

    return FALSE;
}

static Bool
stackswitchInitiate (CompScreen      *s,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int	     nOption)
{
    CompWindow *w;
    CompMatch  *match;
    int        count;

    STACKSWITCH_SCREEN (s);

    if (otherScreenGrabExist (s, "stackswitch", NULL))
	return FALSE;

    ss->currentMatch = stackswitchGetWindowMatch (s);

    match = getMatchOptionNamed (option, nOption, "match", NULL);
    if (match)
    {
	matchFini (&ss->match);
	matchInit (&ss->match);
	if (matchCopy (&ss->match, match))
	{
	    matchUpdate (s->display, &ss->match);
	    ss->currentMatch = &ss->match;
	}
    }

    count = stackswitchCountWindows (s);

    if (count < 1)
	return FALSE;

    if (!ss->grabIndex)
    {
	ss->grabIndex = pushScreenGrab (s, s->invisibleCursor, "stackswitch");
    }

    if (ss->grabIndex)
    {
	ss->state = StackswitchStateOut;
	stackswitchActivateEvent (s, TRUE);

	if (!stackswitchCreateWindowList (s))
	    return FALSE;

	ss->selectedWindow = ss->windows[0];
	stackswitchRenderWindowTitle (s);

	for (w = s->windows; w; w = w->next)
	{
	    STACKSWITCH_WINDOW (w);

	    sw->tx = w->attrib.x - w->input.left;
	    sw->ty = w->attrib.y + w->attrib.height + w->input.bottom;
	}
	ss->moreAdjust = TRUE;
	damageScreen (s);
    }

    return TRUE;
}

static Bool
stackswitchDoSwitch (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption,
		     Bool            nextWindow,
		     StackswitchType type)
{
    CompScreen *s;
    Window     xid;
    Bool       ret = TRUE;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	STACKSWITCH_SCREEN (s);

	if ((ss->state == StackswitchStateNone) || (ss->state == StackswitchStateIn))
	{
	    if (type == StackswitchTypeGroup)
	    {
		CompWindow *w;
		w = findWindowAtDisplay (d, getIntOptionNamed (option, nOption,
							       "window", 0));
		if (w)
		{
		    ss->type = StackswitchTypeGroup;
		    ss->clientLeader =
			(w->clientLeader) ? w->clientLeader : w->id;
		    ret = stackswitchInitiate (s, action, state, option, nOption);
		}
	    }
	    else
	    {
		ss->type = type;
		ret = stackswitchInitiate (s, action, state, option, nOption);
	    }

	    if (state & CompActionStateInitKey)
		action->state |= CompActionStateTermKey;

	    if (state & CompActionStateInitEdge)
		action->state |= CompActionStateTermEdge;
	    else if (state & CompActionStateInitButton)
		action->state |= CompActionStateTermButton;
	}

	if (ret)
	    switchToWindow (s, nextWindow);
    }

    return ret;
}

static Bool
stackswitchNext (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int	         nOption)
{
    return stackswitchDoSwitch (d, action, state, option, nOption,
			 TRUE, StackswitchTypeNormal);
}

static Bool
stackswitchPrev (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int	         nOption)
{
    return stackswitchDoSwitch (d, action, state, option, nOption,
			 FALSE, StackswitchTypeNormal);
}

static Bool
stackswitchNextAll (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int	            nOption)
{
    return stackswitchDoSwitch (d, action, state, option, nOption,
			 TRUE, StackswitchTypeAll);
}

static Bool
stackswitchPrevAll (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int	            nOption)
{
    return stackswitchDoSwitch (d, action, state, option, nOption,
			 FALSE, StackswitchTypeAll);
}

static Bool
stackswitchNextGroup (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int	      nOption)
{
    return stackswitchDoSwitch (d, action, state, option, nOption,
			 TRUE, StackswitchTypeGroup);
}

static Bool
stackswitchPrevGroup (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int	      nOption)
{
    return stackswitchDoSwitch (d, action, state, option, nOption,
			 FALSE, StackswitchTypeGroup);
}


static void
stackswitchWindowRemove (CompDisplay *d,
			 CompWindow  *w)
{
    if (w)
    {
	Bool   inList = FALSE;
	int    j, i = 0;
	CompWindow *selected;

	STACKSWITCH_SCREEN (w->screen);

	if (ss->state == StackswitchStateNone)
	    return;

	if (isStackswitchWin (w))
	    return;

	selected = ss->selectedWindow;

	while (i < ss->nWindows)
	{
	    if (w == ss->windows[i])
	    {
		inList = TRUE;

		if (w == selected)
		{
		    if (i < (ss->nWindows - 1))
			selected = ss->windows[i + 1];
		    else
			selected = ss->windows[0];

		    ss->selectedWindow = selected;
		    stackswitchRenderWindowTitle (w->screen);
		}

		ss->nWindows--;
		for (j = i; j < ss->nWindows; j++)
		    ss->windows[j] = ss->windows[j + 1];
	    }
	    else
	    {
		i++;
	    }
	}

	if (!inList)
	    return;

	if (ss->nWindows == 0)
	{
	    CompOption o;

	    o.type = CompOptionTypeInt;
	    o.name = "root";
	    o.value.i = w->screen->root;

	    stackswitchTerminate (d, NULL, 0, &o, 1);
	    return;
	}

	// Let the window list be updated to avoid crash
	// when a window is closed while ending (StackswitchStateIn).
	if (!ss->grabIndex && ss->state != StackswitchStateIn)
	    return;

	if (stackswitchUpdateWindowList (w->screen))
	{
	    ss->moreAdjust = TRUE;
	    ss->state = StackswitchStateOut;
	    damageScreen (w->screen);
	}
    }
}

static void
stackswitchHandleEvent (CompDisplay *d,
			XEvent      *event)
{
    CompWindow *w = NULL;

    switch (event->type) {
    case DestroyNotify:
	/* We need to get the CompWindow * for event->xdestroywindow.window
	   here because in the (*d->handleEvent) call below, that CompWindow's
	   id will become 1, so findWindowAtDisplay won't be able to find the
	   CompWindow after that. */
	w = findWindowAtDisplay (d, event->xdestroywindow.window);
	break;
    }

    STACKSWITCH_DISPLAY (d);

    UNWRAP (sd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (sd, d, handleEvent, stackswitchHandleEvent);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == XA_WM_NAME)
	{
	    w = findWindowAtDisplay (d, event->xproperty.window);
	    if (w)
	    {
		STACKSWITCH_SCREEN (w->screen);
		if (ss->grabIndex && (w == ss->selectedWindow))
		{
		    stackswitchRenderWindowTitle (w->screen);
		    damageScreen (w->screen);
		}
	    }
	}
	break;
    case UnmapNotify:
	w = findWindowAtDisplay (d, event->xunmap.window);
	stackswitchWindowRemove (d, w);
	break;
    case DestroyNotify:
	stackswitchWindowRemove (d, w);
	break;
    }
}

static Bool
stackswitchDamageWindowRect (CompWindow *w,
			     Bool	initial,
			     BoxPtr     rect)
{
    Bool       status = FALSE;
    CompScreen *s = w->screen;

    STACKSWITCH_SCREEN (s);

    if (initial)
    {
	if (ss->grabIndex && isStackswitchWin (w))
	{
	    stackswitchAddWindowToList (s, w);
	    if (stackswitchUpdateWindowList (s))
	    {
		STACKSWITCH_WINDOW (w);

		sw->adjust = TRUE;
		ss->moreAdjust = TRUE;
		ss->state = StackswitchStateOut;
		damageScreen (s);
	    }
	}
    }
    else if (ss->state == StackswitchStateSwitching)
    {
	STACKSWITCH_WINDOW (w);

	if (sw->slot)
	{
	    damageScreen (s);
	    status = TRUE;
	}
    }

    UNWRAP (ss, s, damageWindowRect);
    status |= (*s->damageWindowRect) (w, initial, rect);
    WRAP (ss, s, damageWindowRect, stackswitchDamageWindowRect);

    return status;
}

static Bool
stackswitchInitDisplay (CompPlugin  *p,
			CompDisplay *d)
{
    StackswitchDisplay *sd;
    int                index;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    sd = malloc (sizeof (StackswitchDisplay));
    if (!sd)
	return FALSE;

    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (sd->screenPrivateIndex < 0)
    {
	free (sd);
	return FALSE;
    }

    if (checkPluginABI ("text", TEXT_ABIVERSION) &&
	getPluginDisplayIndex (d, "text", &index))
    {
	sd->textFunc = d->base.privates[index].ptr;
    }
    else
    {
	compLogMessage ("stackswitch", CompLogLevelWarn,
			"No compatible text plugin found.");
	sd->textFunc = NULL;
    }

    stackswitchSetNextKeyInitiate (d, stackswitchNext);
    stackswitchSetNextKeyTerminate (d, stackswitchTerminate);
    stackswitchSetPrevKeyInitiate (d, stackswitchPrev);
    stackswitchSetPrevKeyTerminate (d, stackswitchTerminate);
    stackswitchSetNextAllKeyInitiate (d, stackswitchNextAll);
    stackswitchSetNextAllKeyTerminate (d, stackswitchTerminate);
    stackswitchSetPrevAllKeyInitiate (d, stackswitchPrevAll);
    stackswitchSetPrevAllKeyTerminate (d, stackswitchTerminate);
    stackswitchSetNextGroupKeyInitiate (d, stackswitchNextGroup);
    stackswitchSetNextGroupKeyTerminate (d, stackswitchTerminate);
    stackswitchSetPrevGroupKeyInitiate (d, stackswitchPrevGroup);
    stackswitchSetPrevGroupKeyTerminate (d, stackswitchTerminate);

    stackswitchSetNextButtonInitiate (d, stackswitchNext);
    stackswitchSetNextButtonTerminate (d, stackswitchTerminate);
    stackswitchSetPrevButtonInitiate (d, stackswitchPrev);
    stackswitchSetPrevButtonTerminate (d, stackswitchTerminate);
    stackswitchSetNextAllButtonInitiate (d, stackswitchNextAll);
    stackswitchSetNextAllButtonTerminate (d, stackswitchTerminate);
    stackswitchSetPrevAllButtonInitiate (d, stackswitchPrevAll);
    stackswitchSetPrevAllButtonTerminate (d, stackswitchTerminate);
    stackswitchSetNextGroupButtonInitiate (d, stackswitchNextGroup);
    stackswitchSetNextGroupButtonTerminate (d, stackswitchTerminate);
    stackswitchSetPrevGroupButtonInitiate (d, stackswitchPrevGroup);
    stackswitchSetPrevGroupButtonTerminate (d, stackswitchTerminate);

    WRAP (sd, d, handleEvent, stackswitchHandleEvent);

    d->base.privates[StackswitchDisplayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
stackswitchFiniDisplay (CompPlugin  *p,
			CompDisplay *d)
{
    STACKSWITCH_DISPLAY (d);

    freeScreenPrivateIndex (d, sd->screenPrivateIndex);

    UNWRAP (sd, d, handleEvent);

    free (sd);
}

static Bool
stackswitchInitScreen (CompPlugin *p,
		       CompScreen *s)
{
    StackswitchScreen *ss;

    STACKSWITCH_DISPLAY (s->display);

    ss = malloc (sizeof (StackswitchScreen));
    if (!ss)
	return FALSE;

    ss->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ss->windowPrivateIndex < 0)
    {
	free (ss);
	return FALSE;
    }

    ss->grabIndex = 0;

    ss->state = StackswitchStateNone;

    ss->windows     = NULL;
    ss->drawSlots   = NULL;
    ss->windowsSize = 0;

    ss->paintingSwitcher = FALSE;

    ss->selectedWindow = NULL;

    ss->moreAdjust   = FALSE;
    ss->rotateAdjust = FALSE;

    ss->rVelocity = 0;
    ss->rotation  = 0.0;

    ss->textData = NULL;

    matchInit (&ss->match);

    WRAP (ss, s, preparePaintScreen, stackswitchPreparePaintScreen);
    WRAP (ss, s, donePaintScreen, stackswitchDonePaintScreen);
    WRAP (ss, s, paintOutput, stackswitchPaintOutput);
    WRAP (ss, s, paintWindow, stackswitchPaintWindow);
    WRAP (ss, s, damageWindowRect, stackswitchDamageWindowRect);

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    return TRUE;
}

static void
stackswitchFiniScreen (CompPlugin *p,
		       CompScreen *s)
{
    STACKSWITCH_SCREEN (s);

    freeWindowPrivateIndex (s, ss->windowPrivateIndex);

    UNWRAP (ss, s, preparePaintScreen);
    UNWRAP (ss, s, donePaintScreen);
    UNWRAP (ss, s, paintOutput);
    UNWRAP (ss, s, paintWindow);
    UNWRAP (ss, s, damageWindowRect);

    matchFini (&ss->match);

    stackswitchFreeWindowTitle (s);

    if (ss->windows)
	free (ss->windows);

    if (ss->drawSlots)
	free (ss->drawSlots);

    free (ss);
}

static Bool
stackswitchInitWindow (CompPlugin *p,
		       CompWindow *w)
{
    StackswitchWindow *sw;

    STACKSWITCH_SCREEN (w->screen);

    sw = malloc (sizeof (StackswitchWindow));
    if (!sw)
	return FALSE;

    sw->slot          = 0;
    sw->scale         = 1.0f;
    sw->tx            = sw->ty = 0.0f;
    sw->adjust        = FALSE;
    sw->xVelocity     = sw->yVelocity = 0.0f;
    sw->scaleVelocity = 0.0f;
    sw->rotation      = 0.0f;
    sw->rotVelocity   = 0.0f;

    w->base.privates[ss->windowPrivateIndex].ptr = sw;

    return TRUE;
}

static void
stackswitchFiniWindow (CompPlugin *p,
		       CompWindow *w)
{
    STACKSWITCH_WINDOW (w);

    if (sw->slot)
	free (sw->slot);

    free (sw);
}

static CompBool
stackswitchInitObject (CompPlugin *p,
		       CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) stackswitchInitDisplay,
	(InitPluginObjectProc) stackswitchInitScreen,
	(InitPluginObjectProc) stackswitchInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
stackswitchFiniObject (CompPlugin *p,
		       CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) stackswitchFiniDisplay,
	(FiniPluginObjectProc) stackswitchFiniScreen,
	(FiniPluginObjectProc) stackswitchFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
stackswitchInit (CompPlugin *p)
{
    StackswitchDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (StackswitchDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
stackswitchFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (StackswitchDisplayPrivateIndex);
}

CompPluginVTable stackswitchVTable = {
    "stackswitch",
    0,
    stackswitchInit,
    stackswitchFini,
    stackswitchInitObject,
    stackswitchFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &stackswitchVTable;
}
