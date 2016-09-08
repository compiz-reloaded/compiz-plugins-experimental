/*
 * Compiz Static Windows plugin
 *
 * static.c
 *
 * Copyright : (C) 2008 by Mark Thomas
 * E-mail    : markbt@efaref.net
 *
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

#include <string.h>
#include <stdlib.h>
#include <compiz-core.h>

#include "static_options.h"

static int displayPrivateIndex;

typedef struct _StaticDisplay
{
    int screenPrivateIndex;
}
StaticDisplay;

typedef enum _StaticMode
{
    STATIC_ALL,
    STATIC_NORMAL,
    STATIC_STATIC
}
StaticMode;

typedef struct _StaticScreen
{
    int    windowPrivateIndex;

    PaintWindowProc             paintWindow;
    AddWindowGeometryProc       addWindowGeometry;
    DrawWindowTextureProc       drawWindowTexture;
    DamageWindowRectProc        damageWindowRect;
    PaintOutputProc             paintOutput;
    ApplyScreenTransformProc    applyScreenTransform;
    PaintTransformedOutputProc  paintTransformedOutput;
    PreparePaintScreenProc      preparePaintScreen;

    StaticMode   staticMode;
}
StaticScreen;

typedef struct _StaticWindow
{
    FragmentAttrib fragment;
} StaticWindow;

#define GET_STATIC_DISPLAY(d) \
    ((StaticDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define STATIC_DISPLAY(d) \
    StaticDisplay *sd = GET_STATIC_DISPLAY(d);

#define GET_STATIC_SCREEN(s, sd) \
    ((StaticScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)
#define STATIC_SCREEN(s) \
    StaticScreen *ss = GET_STATIC_SCREEN(s, GET_STATIC_DISPLAY(s->display))

#define GET_STATIC_WINDOW(w, ss) \
    ((StaticWindow *) (w)->base.privates[(ss)->windowPrivateIndex].ptr)
#define STATIC_WINDOW(w)                                      \
    StaticWindow *sw = GET_STATIC_WINDOW  (w,                    \
                    GET_STATIC_SCREEN  (w->screen,            \
                    GET_STATIC_DISPLAY (w->screen->display)))

static Bool
isStatic(CompWindow *w)
{
  return matchEval(staticGetWindowMatch(w->screen), w);
}

static void
staticPreparePaintScreen (CompScreen *s,
                          int         msSinceLastPaint)
{
    STATIC_SCREEN(s);

    /* Initially we are painting all windows.
     * This may change if we switch to transformed mode.
     */
    ss->staticMode = STATIC_ALL;

    UNWRAP (ss, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ss, s, preparePaintScreen, staticPreparePaintScreen);
}


static Bool
staticPaintWindow (CompWindow              *w,
                   const WindowPaintAttrib *attrib,
                   const CompTransform     *transform,
                   Region                  region,
                   unsigned int            mask)
{
    CompScreen *s = w->screen;
    Bool status = TRUE;
    STATIC_SCREEN (s);

    if (ss->staticMode == STATIC_STATIC && !isStatic(w))
    {
        /* We are not painting normal windows */
        return FALSE;
    }

    if (ss->staticMode == STATIC_NORMAL && isStatic(w))
    {
        /* We are not painting static windows */
        return FALSE;
    }

    UNWRAP (ss, s, paintWindow);
    status = (*s->paintWindow) (w, attrib, transform, region, mask);
    WRAP (ss, s, paintWindow, staticPaintWindow);

    return status;
}

static void
staticAddWindowGeometry (CompWindow *w,
                         CompMatrix *matrix,
                         int        nMatrix,
                         Region     region,
                         Region     clip)
{
    CompScreen *s = w->screen;
    STATIC_SCREEN (s);

    if (ss->staticMode == STATIC_STATIC && isStatic(w)) {
        addWindowGeometry (w, matrix, nMatrix, region, clip);
        return;
    }

    UNWRAP (ss, s, addWindowGeometry);
    (*s->addWindowGeometry) (w, matrix, nMatrix, region, clip);
    WRAP (ss, s, addWindowGeometry, staticAddWindowGeometry);
}

static void
staticDrawWindowTexture (CompWindow              *w,
                         CompTexture          *texture,
                         const FragmentAttrib *fragment,
                         unsigned int         mask)
{
    CompScreen *s = w->screen;
    STATIC_SCREEN (s);

    if (ss->staticMode == STATIC_ALL && isStatic(w)) {

        STATIC_WINDOW (w);
        memcpy(&sw->fragment, fragment, sizeof (FragmentAttrib));
    }

    if (ss->staticMode == STATIC_STATIC && !isStatic(w))
    {
        /* We are not painting normal windows */
        return;
    }
    if (ss->staticMode == STATIC_NORMAL && isStatic(w))
    {
        /* We are not painting static windows */
        return;
    }

    if (ss->staticMode == STATIC_STATIC && isStatic(w)) {
        STATIC_WINDOW (w);
        drawWindowTexture (w, texture, &sw->fragment, mask);
    } else {
        UNWRAP (ss, s, drawWindowTexture);
        (*s->drawWindowTexture) (w, texture, fragment, mask);
        WRAP (ss, s, drawWindowTexture, staticDrawWindowTexture);
    }
}

static Bool
staticDamageWindowRect (CompWindow *w,
                        Bool       initial,
                        BoxPtr     rect)
{
    CompScreen *s = w->screen;
    Bool status;

    STATIC_SCREEN (s);

    UNWRAP (ss, s, damageWindowRect);
    status = (*s->damageWindowRect) (w, initial, rect);
    WRAP (ss, s, damageWindowRect, staticDamageWindowRect);

    damageScreen (s);

    return status;
}

static void
staticApplyScreenTransform (CompScreen              *s,
                            const ScreenPaintAttrib *sAttrib,
                            CompOutput              *output,
                            CompTransform           *transform)
{
    STATIC_SCREEN(s);

    if (ss->staticMode == STATIC_STATIC)
    {
        applyScreenTransform (s, sAttrib, output, transform);
    }
    else
    {
        UNWRAP (ss, s, applyScreenTransform);
        (*s->applyScreenTransform) (s, sAttrib, output, transform);
        WRAP (ss, s, applyScreenTransform, staticApplyScreenTransform);
    }
}

static void
staticPaintTransformedOutput (CompScreen              *s,
                              const ScreenPaintAttrib *sAttrib,
                              const CompTransform     *transform,
                              Region                  region,
                              CompOutput              *output,
                              unsigned int            mask)
{
    STATIC_SCREEN(s);

    /* We are now painting in transformed mode.
     * Start painting only normal windows.
     */
    ss->staticMode = STATIC_NORMAL;

    UNWRAP (ss, s, paintTransformedOutput);
    (*s->paintTransformedOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (ss, s, paintTransformedOutput, staticPaintTransformedOutput);
}

static Bool
staticPaintOutput(CompScreen              *s,
                  const ScreenPaintAttrib *sAttrib,
                  const CompTransform     *transform,
                  Region                  region,
                  CompOutput              *output,
                  unsigned int            mask)
{
    Bool status;

    STATIC_SCREEN (s);

    UNWRAP (ss, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region,
                                output, mask);
    WRAP (ss, s, paintOutput, staticPaintOutput);

    if ( ss->staticMode == STATIC_NORMAL )
    {
        CompTransform sTransform;

        /* We switched to Transformed mode somewhere along the line.
         * Now paint the dock windows untransformed by bypassing
         * the virtual table.
         */
        mask &= ~PAINT_SCREEN_CLEAR_MASK;

        ss->staticMode = STATIC_STATIC;

        matrixGetIdentity (&sTransform);

        paintTransformedOutput (s, sAttrib, &sTransform, &s->region,
                                output, mask);

        ss->staticMode = STATIC_NORMAL;
    }

    return status;
}


static Bool
staticInitScreen (CompPlugin *p,
                  CompScreen *s)
{
    StaticScreen *ss;

    STATIC_DISPLAY (s->display);

    ss = calloc (1, sizeof (StaticScreen) );

    if (!ss)
        return FALSE;

    ss->windowPrivateIndex = allocateWindowPrivateIndex (s);

    if (ss->windowPrivateIndex < 0)
    {
        free (ss);
        return FALSE;
    }

    WRAP (ss, s, paintWindow, staticPaintWindow);
    WRAP (ss, s, paintOutput, staticPaintOutput);
    WRAP (ss, s, damageWindowRect, staticDamageWindowRect);
    WRAP (ss, s, addWindowGeometry, staticAddWindowGeometry);
    WRAP (ss, s, drawWindowTexture, staticDrawWindowTexture);
    WRAP (ss, s, preparePaintScreen, staticPreparePaintScreen);
    WRAP (ss, s, applyScreenTransform, staticApplyScreenTransform);
    WRAP (ss, s, paintTransformedOutput, staticPaintTransformedOutput);

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    return TRUE;
}

static void
staticFiniScreen (CompPlugin *p,
                  CompScreen *s)
{
    STATIC_SCREEN (s);

    freeWindowPrivateIndex (s, ss->windowPrivateIndex);

    UNWRAP (ss, s, paintTransformedOutput);
    UNWRAP (ss, s, applyScreenTransform);
    UNWRAP (ss, s, preparePaintScreen);
    UNWRAP (ss, s, drawWindowTexture);
    UNWRAP (ss, s, addWindowGeometry);
    UNWRAP (ss, s, damageWindowRect);
    UNWRAP (ss, s, paintOutput);
    UNWRAP (ss, s, paintWindow);

    free (ss);
}

static Bool
staticInitDisplay (CompPlugin *p,
                       CompDisplay *d)
{
    StaticDisplay *sd;

    sd = malloc (sizeof (StaticDisplay) );

    if (!sd)
        return FALSE;

    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (sd->screenPrivateIndex < 0)
    {
        free (sd);
        return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
staticFiniDisplay (CompPlugin *p,
                   CompDisplay *d)
{
    STATIC_DISPLAY (d);

    freeScreenPrivateIndex (d, sd->screenPrivateIndex);

    free (sd);
}

static Bool
staticInitWindow (CompPlugin *p,
                  CompWindow *w)
{
    StaticWindow *sw;

    STATIC_SCREEN (w->screen);

    sw = malloc (sizeof (StaticWindow));
    if (!sw)
        return FALSE;

    initFragmentAttrib (&sw->fragment, &w->paint);

    w->base.privates[ss->windowPrivateIndex].ptr = sw;

    return TRUE;
}

static void
staticFiniWindow (CompPlugin *p,
                  CompWindow *w)
{
    STATIC_WINDOW (w);

    free (sw);
}

static Bool
staticInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex();

    if (displayPrivateIndex < 0)
        return FALSE;

    return TRUE;
}

static void
staticFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
        freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
staticInitObject (CompPlugin *p,
		  CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) staticInitDisplay,
	(InitPluginObjectProc) staticInitScreen,
	(InitPluginObjectProc) staticInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
staticFiniObject (CompPlugin *p,
		  CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) staticFiniDisplay,
	(FiniPluginObjectProc) staticFiniScreen,
	(FiniPluginObjectProc) staticFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompPluginVTable staticVTable = {
    "static",
    0,
    staticInit,
    staticFini,
    staticInitObject,
    staticFiniObject,
    0,
    0
};


CompPluginVTable *
getCompPluginInfo (void)
{
    return &staticVTable;
}
