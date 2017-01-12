/**
 * Compiz Fusion Freewins plugin
 *
 * freewins.c
 *
 * Copyright (C) 2007  Rodolfo Granata <warlock.cc@gmail.com>
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
 * Rodolfo Granata <warlock.cc@gmail.com>
 * Sam Spilsbury <smspillaz@gmail.com>
 *
 * Button binding support and Reset added by:
 * enigma_0Z <enigma.0ZA@gmail.com>
 *
 * Most of the input handling here is based on
 * the shelf plugin by
 *        : Kristian Lyngstøl <kristian@bohemians.org>
 *        : Danny Baumann <maniac@opencompositing.org>
 *
 * Description:
 *
 * This plugin allows you to freely transform the texture of a window,
 * whether that be rotation or scaling to make better use of screen space
 * or just as a toy.
 *
 * Todo:
 *  - Modifier key to rotate on the Z Axis
 *  - Fully implement an input redirection system by
 *    finding an inverse matrix, multiplying by it,
 *    translating to the actual window co-ords and
 *    XSendEvent() the co-ords to the actual window.
 *  - Code could be cleaner
 *  - Add timestep and speed options to animation
 *  - Add window hover-over info via paintOutput : i.e
 *    - Resize borders
 *    - 'Reset' Button
 *    - 'Scale' Button
 *    - 'Rotate' Button
 */

#include "freewins.h"

static CompMetadata freewinsMetadata;

/* Information on window resize */
void
FWWindowResizeNotify(CompWindow *w,
                    int dx,
                    int dy,
                    int dw,
                    int dh)
{
    FREEWINS_WINDOW (w);
    FREEWINS_SCREEN (w->screen);

    FWCalculateInputRect (w);

    int x = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0;
    int y = WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0;

    fww->radius = sqrt(pow((x - WIN_REAL_X (w)), 2) + pow((y - WIN_REAL_Y (w)), 2));

    UNWRAP(fws, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify)(w, dx, dy, dw, dh);
    WRAP(fws, w->screen, windowResizeNotify, FWWindowResizeNotify);
}

void
FWWindowMoveNotify (CompWindow *w,
		       int        dx,
		       int        dy,
		       Bool       immediate)
{
    FREEWINS_DISPLAY (w->screen->display);
    FREEWINS_SCREEN (w->screen);
    FREEWINS_WINDOW (w);

    CompWindow *useWindow;

    FWCalculateInputRect (w);

    useWindow = FWGetRealWindow (w); /* Did we move an IPW and not the actual window? */
    if (useWindow)
        moveWindow (useWindow, dx, dy, TRUE, freewinsGetImmediateMoves (w->screen));
    else if (w != fwd->grabWindow)
        FWAdjustIPW (w); /* We moved a window but not the IPW, so adjust it */

	int x = WIN_REAL_X (w) + WIN_REAL_W (w) /2.0;
	int y = WIN_REAL_Y (w) + WIN_REAL_H (w) /2.0;

    fww->radius = sqrt (pow((x - WIN_REAL_X (w)), 2) + pow ((y - WIN_REAL_Y (w)), 2));

    UNWRAP (fws, w->screen, windowMoveNotify);
    (*w->screen->windowMoveNotify) (w, dx, dy, immediate);
    WRAP (fws, w->screen, windowMoveNotify, FWWindowMoveNotify);
}

static void
FWReloadSnapKeys (CompDisplay *d)
{
    FREEWINS_DISPLAY (d);

    if (fwd)
    {
	unsigned int imask = freewinsGetInvertModsMask(d);
	fwd->invertMask = 0;

	if (imask & InvertModsShiftMask)
		fwd->invertMask |= ShiftMask;
	if (imask & InvertModsAltMask)
		fwd->invertMask |= CompAltMask;
	if (imask & InvertModsControlMask)
		fwd->invertMask |= ControlMask;
	if (imask & InvertModsMetaMask)
		fwd->invertMask |= CompMetaMask;

	unsigned int smask = freewinsGetSnapModsMask(d);
	fwd->snapMask = 0;
	if (smask & SnapModsShiftMask)
		fwd->snapMask |= ShiftMask;
	if (smask & SnapModsAltMask)
		fwd->snapMask |= CompAltMask;
	if (smask & SnapModsControlMask)
		fwd->snapMask |= ControlMask;
	if (smask & SnapModsMetaMask)
		fwd->snapMask |= CompMetaMask;

    }
}

static void
FWDisplayOptionChanged (CompDisplay *d,
                        CompOption *opt,
                        FreewinsDisplayOptions num)
{
    switch (num)
    {
	case FreewinsDisplayOptionSnapMods:
        case FreewinsDisplayOptionInvertMods:
	    FWReloadSnapKeys (d);
            break;
	default:
	    break;
    }
}

/* ------ Plugin Initialization ---------------------------------------*/

/* Window initialization / cleaning */
static Bool
freewinsInitWindow (CompPlugin *p,
		    CompWindow *w)
{
    FWWindow *fww;
    FREEWINS_SCREEN(w->screen);

    if (!(fww = (FWWindow*) malloc ( sizeof (FWWindow))))
	return FALSE;

    fww->transform.angX = 0.0;
    fww->transform.angY = 0.0;
    fww->transform.angZ = 0.0;

    fww->iMidX = WIN_REAL_W (w) /2.0;
    fww->iMidY = WIN_REAL_H (w) /2.0;

    fww->adjustX = 0.0f;
    fww->adjustY = 0.0f;

    int x = WIN_REAL_X (w) + WIN_REAL_W (w) /2.0;
    int y = WIN_REAL_Y (w) + WIN_REAL_H (w) /2.0;

    fww->radius = sqrt (pow ((x - WIN_REAL_X (w)), 2) + pow ((y - WIN_REAL_Y (w)), 2));

    fww->outputRect.x1 = WIN_OUTPUT_X (w);
    fww->outputRect.x2 = WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w);
    fww->outputRect.y1 = WIN_OUTPUT_Y (w);
    fww->outputRect.y2 = WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w);

    fww->grab = grabNone;
    fww->can2D = FALSE;
    fww->can3D = FALSE;

    fww->transformed = FALSE;

    // Don't allow anything yet
    fww->resetting = FALSE;
    fww->isAnimating = FALSE;

    // Don't allow incorrect window drawing as soon as the plugin is started

    fww->transform.scaleX = 1.0;
    fww->transform.scaleY = 1.0;

    fww->transform.unsnapScaleX = 1.0;
    fww->transform.unsnapScaleY = 1.0;

    fww->animate.destAngX = 0.0f;
    fww->animate.destAngY = 0.0f;
    fww->animate.destAngZ = 0.0f;
    fww->animate.destScaleX = 1.0f;
    fww->animate.destScaleY = 1.0f;

    fww->animate.oldAngX = 0.0f;
    fww->animate.oldAngY = 0.0f;
    fww->animate.oldAngZ = 0.0f;
    fww->animate.oldScaleX = 1.0f;
    fww->animate.oldScaleY = 01.0f;

    fww->input = NULL;

    w->base.privates[fws->windowPrivateIndex].ptr = fww;

    return TRUE;
}

static void
freewinsFiniWindow (CompPlugin *p,
		    CompWindow *w)
 {

    FREEWINS_WINDOW (w);
    FREEWINS_DISPLAY (w->screen->display);

    /* Shape window back to normal */
    fww->transform.scaleX = 1.0f;
    fww->transform.scaleY = 1.0f;

    fww->transform.angX   = 0.0f;
    fww->transform.angY   = 0.0f;
    fww->transform.angZ   = 0.0f;

    fww->transformed = FALSE;

    if (FWCanShape (w))
        FWHandleWindowInputInfo (w);

    if (fwd->grabWindow == w)
	fwd->grabWindow = NULL;

   free(fww);
}

/* Screen initialization / cleaning */
static Bool
freewinsInitScreen (CompPlugin *p,
		    CompScreen *s)
{
    FWScreen *fws;

    FREEWINS_DISPLAY(s->display);

    if (!(fws = (FWScreen*) malloc ( sizeof (FWScreen))))
	return FALSE;

    if ((fws->windowPrivateIndex = allocateWindowPrivateIndex (s)) < 0)
    {
	free(fws);
	return FALSE;
    }

    fws->grabIndex = 0;
    fws->transformedWindows = NULL;

    s->base.privates[fwd->screenPrivateIndex].ptr = fws;

    WRAP (fws, s, preparePaintScreen, FWPreparePaintScreen);
    WRAP (fws, s, paintWindow, FWPaintWindow);
    WRAP (fws, s, paintOutput, FWPaintOutput);
    WRAP (fws, s, donePaintScreen, FWDonePaintScreen);

    WRAP (fws, s, damageWindowRect, FWDamageWindowRect);

    WRAP (fws, s, windowResizeNotify, FWWindowResizeNotify);
    WRAP (fws, s, windowMoveNotify, FWWindowMoveNotify);

    return TRUE;
}

static void
freewinsFiniScreen (CompPlugin *p,
                    CompScreen *s)
{

    FREEWINS_SCREEN(s);

    freeWindowPrivateIndex (s, fws->windowPrivateIndex);

    UNWRAP (fws, s, preparePaintScreen);
    UNWRAP (fws, s, paintWindow);
    UNWRAP (fws, s, paintOutput);
    UNWRAP (fws, s, donePaintScreen);

    UNWRAP (fws, s, damageWindowRect);

    UNWRAP (fws, s, windowResizeNotify);
    UNWRAP (fws, s, windowMoveNotify);

    free (fws);
}

/* Display initialization / cleaning */
static Bool
freewinsInitDisplay (CompPlugin *p,
                     CompDisplay *d)
{

    FWDisplay *fwd;

    if (!(fwd = (FWDisplay*) malloc (sizeof (FWDisplay))))
	return FALSE;

    // Set variables correctly
    fwd->grabWindow = 0;
    fwd->lastGrabWindow = 0;
    fwd->axisHelp = FALSE;
    fwd->hoverWindow = 0;

    if ((fwd->screenPrivateIndex = allocateScreenPrivateIndex (d)) < 0 )
    {
	free(fwd);
	return FALSE;
    }

    // Spit out a warning if there is no shape extension
    if (!d->shapeExtension)
        compLogMessage ("freewins", CompLogLevelInfo, "No input shaping extension. Input shaping disabled");


    /* BCOP Action initiation */
    freewinsSetInitiateRotationButtonInitiate (d, initiateFWRotate);
    freewinsSetInitiateRotationButtonTerminate (d, terminateFWRotate);
    freewinsSetInitiateScaleButtonInitiate (d, initiateFWScale);
    freewinsSetInitiateScaleButtonTerminate (d, terminateFWScale);
    freewinsSetResetButtonInitiate (d, resetFWTransform);
    freewinsSetResetKeyInitiate (d, resetFWTransform);
    freewinsSetToggleAxisKeyInitiate (d, toggleFWAxis);

    // Rotate / Scale Up Down Left Right

    freewinsSetScaleUpButtonInitiate (d, FWScaleUp);
    freewinsSetScaleDownButtonInitiate (d, FWScaleDown);
    freewinsSetScaleUpKeyInitiate (d, FWScaleUp);
    freewinsSetScaleDownKeyInitiate (d, FWScaleDown);

    freewinsSetRotateUpKeyInitiate (d, FWRotateUp);
    freewinsSetRotateDownKeyInitiate (d, FWRotateDown);
    freewinsSetRotateLeftKeyInitiate (d, FWRotateLeft);
    freewinsSetRotateRightKeyInitiate (d, FWRotateRight);
    freewinsSetRotateCKeyInitiate (d, FWRotateClockwise);
    freewinsSetRotateCcKeyInitiate (d, FWRotateCounterclockwise);

    freewinsSetRotateInitiate (d, freewinsRotateWindow);
    freewinsSetIncrementRotateInitiate (d, freewinsIncrementRotateWindow);
    freewinsSetScaleInitiate (d, freewinsScaleWindow);

    freewinsSetSnapModsNotify (d, FWDisplayOptionChanged);
    freewinsSetInvertModsNotify (d, FWDisplayOptionChanged);

    d->base.privates[displayPrivateIndex].ptr = fwd;
    WRAP(fwd, d, handleEvent, FWHandleEvent);

    FWReloadSnapKeys (d);

    return TRUE;
}

static void
freewinsFiniDisplay (CompPlugin *p,
                    CompDisplay *d)
{

    FREEWINS_DISPLAY(d);

    freeScreenPrivateIndex(d, fwd->screenPrivateIndex);

    UNWRAP(fwd, d, handleEvent);

    free(fwd);
}

/* Object Initiation and Finitialization */

static CompBool
freewinsInitObject (CompPlugin *p,
		            CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0,
	(InitPluginObjectProc) freewinsInitDisplay,
	(InitPluginObjectProc) freewinsInitScreen,
	(InitPluginObjectProc) freewinsInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
freewinsFiniObject (CompPlugin *p,
		            CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0,
	(FiniPluginObjectProc) freewinsFiniDisplay,
	(FiniPluginObjectProc) freewinsFiniScreen,
	(FiniPluginObjectProc) freewinsFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}


/* Plugin initialization / cleaning */
static Bool
freewinsInit (CompPlugin *p)
{

    if ((displayPrivateIndex = allocateDisplayPrivateIndex ()) < 0 )
	return FALSE;

    compAddMetadataFromFile (&freewinsMetadata, p->vTable->name);

    return TRUE;
}


static void
freewinsFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex( displayPrivateIndex );
}

/* Plugin implementation export */
CompPluginVTable freewinsVTable = {
    "freewins",
    0,
    freewinsInit,
    freewinsFini,
    freewinsInitObject,
    freewinsFiniObject,
    0,
    0,
};

CompPluginVTable *getCompPluginInfo (void){ return &freewinsVTable; }
