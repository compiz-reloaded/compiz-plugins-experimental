/**
 * Compiz Fusion Freewins plugin
 *
 * action.c
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

/* TODO: Finish porting stuff to actions */

#include "freewins.h"


/* ------ Actions -------------------------------------------------------*/

/* Initiate Mouse Rotation */
Bool
initiateFWRotate (CompDisplay *d,
                  CompAction *action,
                  CompActionState state,
                  CompOption *option,
                  int nOption)
{
    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    int x, y, mods;

    FREEWINS_DISPLAY (d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    useW = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s && w)
    {

    FREEWINS_SCREEN (s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (info->ipw)
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

    fws->rotateCursor = XCreateFontCursor (s->display->display, XC_fleur);

    if (!otherScreenGrabExist (s, "freewins", 0))
	if (!fws->grabIndex)
	    fws->grabIndex = pushScreenGrab (s, fws->rotateCursor, "freewins");

    }

    if (useW)
    {

	if (matchEval (freewinsGetShapeWindowTypes (useW->screen), useW))
	{
	    FREEWINS_WINDOW (useW);

	    x = getIntOptionNamed (option, nOption, "x",
					useW->attrib.x + (useW->width / 2));
	    y = getIntOptionNamed (option, nOption, "y",
					useW->attrib.y + (useW->height / 2));

	    mods = getIntOptionNamed (option, nOption, "modifiers", 0);

	    fwd->grabWindow = useW;

	    fww->grab = grabRotate;

	    /* Save current scales and angles */

	    fww->animate.oldAngX = fww->transform.angX;
	    fww->animate.oldAngY = fww->transform.angY;
	    fww->animate.oldAngZ = fww->transform.angZ;
	    fww->animate.oldScaleX = fww->transform.scaleX;
	    fww->animate.oldScaleY = fww->transform.scaleY;

	    if (pointerY > fww->iMidY)
	    {
		if (pointerX > fww->iMidX)
			fww->corner = CornerBottomRight;
		else if (pointerX < fww->iMidX)
			fww->corner = CornerBottomLeft;
	    }
	    else if (pointerY < fww->iMidY)
	    {
		if (pointerX > fww->iMidX)
			fww->corner = CornerTopRight;
		else if (pointerX < fww->iMidX)
			fww->corner = CornerTopLeft;
	    }

	   switch (freewinsGetZAxisRotation (s))
	   {
		case ZAxisRotationAlways3d:
		    fww->can3D = TRUE;
		    fww->can2D = FALSE; break;
		case ZAxisRotationAlways2d:
		    fww->can3D = FALSE;
		    fww->can2D = TRUE; break;
		case ZAxisRotationDetermineOnClick:
		case ZAxisRotationSwitch:
		    FWDetermineZAxisClick (useW, pointerX, pointerY, FALSE); break;
		case ZAxisRotationInterchangable:
		    fww->can3D = TRUE;
		    fww->can2D = TRUE;  break;
		default:
		    break;
	    }

	    /* Set the rotation axis */

	    switch (freewinsGetRotationAxis (w->screen))
	    {
		case RotationAxisAlwaysCentre:
		default:
		    FWCalculateInputOrigin (w,
					    WIN_REAL_X (fwd->grabWindow) +
					    WIN_REAL_W (fwd->grabWindow) / 2.0f,
					    WIN_REAL_Y (fwd->grabWindow) +
					    WIN_REAL_H (fwd->grabWindow) / 2.0f);
		    FWCalculateOutputOrigin (w,
					    WIN_OUTPUT_X (fwd->grabWindow) +
					    WIN_OUTPUT_W (fwd->grabWindow) / 2.0f,
					    WIN_OUTPUT_Y (fwd->grabWindow) +
					    WIN_OUTPUT_H (fwd->grabWindow) / 2.0f);
		    break;
		case RotationAxisClickPoint:
		    FWCalculateInputOrigin (fwd->grabWindow, fwd->click_root_x, fwd->click_root_y);
		    FWCalculateOutputOrigin (fwd->grabWindow, fwd->click_root_x, fwd->click_root_y);
		    break;
		case RotationAxisOppositeToClick:
		    FWCalculateInputOrigin (fwd->grabWindow, w->attrib.x + w->width - fwd->click_root_x,
		                              w->attrib.y + w->height - fwd->click_root_y);
		    FWCalculateOutputOrigin (fwd->grabWindow, w->attrib.x + w->width - fwd->click_root_x,
		                              w->attrib.y + w->height - fwd->click_root_y);
		    break;
	    }

	    /* Announce that we grabbed the window */

	    (useW->screen->windowGrabNotify) (useW, x, y, mods,
					   CompWindowGrabMoveMask |
					   CompWindowGrabButtonMask);

	    /* Shape the window beforehand and avoid a stale grab*/
	    if (FWCanShape (useW))
		if (FWHandleWindowInputInfo (useW))
		    FWAdjustIPW (useW);


	    if (state & CompActionStateInitButton)
		action->state |= CompActionStateTermButton;

	    if (state & CompActionStateInitKey)
		action->state |= CompActionStateTermKey;

	}
    }
    return TRUE;
}


Bool
terminateFWRotate (CompDisplay     *d,
	               CompAction      *action,
	               CompActionState state,
	               CompOption      *option,
	               int             nOption)
{

    CompScreen *s;

    FREEWINS_DISPLAY (d);

    for (s = d->screens; s; s = s->next)
    {
	FREEWINS_SCREEN (s);

        if (fwd->grabWindow && fws->grabIndex)
        {
            FREEWINS_WINDOW (fwd->grabWindow);
	    if (fww->grab == grabRotate)
	    {
		int distX, distY;

		(fwd->grabWindow->screen->windowUngrabNotify) (fwd->grabWindow);

		switch (freewinsGetRotationAxis (fwd->grabWindow->screen))
		{
		    case RotationAxisClickPoint:
		    case RotationAxisOppositeToClick:

		    distX =  (fww->outputRect.x1 +
			      (fww->outputRect.x2 - fww->outputRect.x1) / 2.0f) -
			      (WIN_REAL_X (fwd->grabWindow) +
			       WIN_REAL_W (fwd->grabWindow) / 2.0f);
		    distY = (fww->outputRect.y1 +
			     (fww->outputRect.y2 - fww->outputRect.y1) / 2.0f) -
			     (WIN_REAL_Y (fwd->grabWindow) +
			      WIN_REAL_H (fwd->grabWindow) / 2.0f);

		    moveWindow (fwd->grabWindow, distX, distY, TRUE, TRUE);
		    syncWindowPosition (fwd->grabWindow);

	            FWCalculateInputOrigin (fwd->grabWindow, WIN_REAL_X (fwd->grabWindow) +
							     WIN_REAL_W (fwd->grabWindow) / 2.0f,
							     WIN_REAL_Y (fwd->grabWindow) +
							     WIN_REAL_H (fwd->grabWindow) / 2.0f);
		    FWCalculateOutputOrigin(fwd->grabWindow,
					    WIN_OUTPUT_X (fwd->grabWindow) +
					    WIN_OUTPUT_W (fwd->grabWindow) / 2.0f,
					    WIN_OUTPUT_Y (fwd->grabWindow) +
					    WIN_OUTPUT_H (fwd->grabWindow) / 2.0f);

		    break;
		    default:
			break;
		}

		if (FWCanShape (fwd->grabWindow))
		    if (FWHandleWindowInputInfo (fwd->grabWindow))
			FWAdjustIPW (fwd->grabWindow);

		removeScreenGrab(s, fws->grabIndex, 0);
		fws->grabIndex = 0;
		fwd->grabWindow = NULL;
		fww->grab = grabNone;

	    }
	}
    }

    action->state &= ~ (CompActionStateTermKey | CompActionStateTermButton);

    return FALSE;
}

/*static void FWMoveWindowToCorrectPosition (CompWindow *w, float distX, float distY)
{

	FREEWINS_WINDOW (w);

	fprintf(stderr, "distX is %f distY is %f midX and midY are %f %f\n", distX, distY, fww->iMidX, fww->iMidY);

	moveWindow (w, distX * (1 + (1 - fww->transform.scaleX)), distY * (1 + (1 - fww->transform.scaleY)), TRUE, FALSE);

	syncWindowPosition (w);
}*/


/* Initiate Scaling */
Bool
initiateFWScale (CompDisplay *d,
			    CompAction *action,
			    CompActionState state,
			    CompOption *option,
			    int nOption)
{

    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    int x, y, mods;

    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    useW = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s && w && useW)
    {

	FREEWINS_SCREEN (s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_plus);

	if (!otherScreenGrabExist(s, "freewins", 0))
	    if (!fws->grabIndex)
		fws->grabIndex = pushScreenGrab (s, fws->rotateCursor, "freewins");

    }

    if (useW)
    {
	if (matchEval (freewinsGetShapeWindowTypes (useW->screen), useW))
	{

	FREEWINS_WINDOW (useW);

	x = getIntOptionNamed (option, nOption, "x",
				   useW->attrib.x + (useW->width / 2));
	y = getIntOptionNamed (option, nOption, "y",
				   useW->attrib.y + (useW->height / 2));

	mods = getIntOptionNamed (option, nOption, "modifiers", 0);

	fwd->grabWindow = useW;

	/* Find out the corner we clicked in */

	float MidX = fww->inputRect.x1 + ((fww->inputRect.x2 - fww->inputRect.x1) / 2.0f);
	float MidY = fww->inputRect.y1 + ((fww->inputRect.y2 - fww->inputRect.y1) / 2.0f);

	/* Check for Y axis clicking (Top / Bottom) */
	if (pointerY > MidY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (pointerX > MidX)
		fww->corner = CornerBottomRight;
	    else if (pointerX < MidX)
		fww->corner = CornerBottomLeft;
	}
	else if (pointerY < MidY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (pointerX > MidX)
		fww->corner = CornerTopRight;
	    else if (pointerX < MidX)
		fww->corner = CornerTopLeft;
	}


	switch (freewinsGetScaleMode (w->screen))
	{
	    case ScaleModeToCentre:
	        FWCalculateInputOrigin(useW, WIN_REAL_X (w) + WIN_REAL_W (w) / 2.0f,
	                                  WIN_REAL_Y (useW) + WIN_REAL_H (useW) / 2.0f);
	        FWCalculateOutputOrigin(useW, WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w) / 2.0f,
	                                   WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w) / 2.0f);
	        break;
	    /**
	      *Experimental scale to corners mode
	      */
	    case ScaleModeToOppositeCorner:
	        switch (fww->corner)
	        {
	            case CornerBottomRight:
	            /* Translate origin to the top left of the window */
	            //FWMoveWindowToCorrectPosition (w, fww->inputRect.x1 - WIN_REAL_X (useW), fww->inputRect.y1 - WIN_REAL_Y (useW));
	            FWCalculateInputOrigin (useW, WIN_REAL_X (useW), WIN_REAL_Y (useW));
	            break;
	            case CornerBottomLeft:
	            /* Translate origin to the top right of the window */
	            //FWMoveWindowToCorrectPosition (w, fww->inputRect.x2 - (WIN_REAL_X (useW) + WIN_REAL_W (useW)), fww->inputRect.y1 - WIN_REAL_Y (useW));
	            FWCalculateInputOrigin (useW, WIN_REAL_X (useW) + (WIN_REAL_W (useW)), WIN_REAL_Y (useW));
	            break;
	            case CornerTopRight:
	            /* Translate origin to the bottom left of the window */
	            //FWMoveWindowToCorrectPosition (w, fww->inputRect.x1 - WIN_REAL_X (useW) , fww->inputRect.y1 - (WIN_REAL_Y (useW) + WIN_REAL_H (useW)));
	            FWCalculateInputOrigin (useW, WIN_REAL_X (useW), WIN_REAL_Y (useW) + (WIN_REAL_H (useW)));
	            break;
	            case CornerTopLeft:
	            /* Translate origin to the bottom right of the window */
	            //FWMoveWindowToCorrectPosition (w, fww->inputRect.x1 -(WIN_REAL_X (useW) + WIN_REAL_W (useW)) , fww->inputRect.y1 - (WIN_REAL_Y (useW) + WIN_REAL_H (useW)));
	            FWCalculateInputOrigin (useW, WIN_REAL_X (useW) + (WIN_REAL_W (useW)), WIN_REAL_Y (useW) + (WIN_REAL_H (useW)));
	            break;
	        }
	        break;
	}

	fww->grab = grabScale;

	/* Announce that we grabbed the window */

	(w->screen->windowGrabNotify) (w, x, y, mods,
			   CompWindowGrabMoveMask |
			   CompWindowGrabButtonMask);

	/*Shape the window beforehand and avoid a stale grab*/
	if (FWCanShape (useW))
	    if (FWHandleWindowInputInfo (useW))
	        FWAdjustIPW (useW);

	if (state & CompActionStateInitButton)
	    action->state |= CompActionStateTermButton;

	if (state & CompActionStateInitKey)
		action->state |= CompActionStateTermKey;

	}
    }

    return TRUE;
}

Bool
terminateFWScale (CompDisplay     *d,
	               CompAction      *action,
	               CompActionState state,
	               CompOption      *option,
	               int             nOption)
{

    FREEWINS_DISPLAY (d);

    CompScreen *s;

    for (s = d->screens; s; s = s->next)
    {
	FREEWINS_SCREEN (s);

        if (fwd->grabWindow && fws->grabIndex)
        {
            FREEWINS_WINDOW (fwd->grabWindow);
	    if (fww->grab == grabScale)
	    {
		(fwd->grabWindow->screen->windowUngrabNotify) (fwd->grabWindow);

		if (FWCanShape (fwd->grabWindow))
		    if (FWHandleWindowInputInfo (fwd->grabWindow))
			FWAdjustIPW (fwd->grabWindow);

		switch (freewinsGetScaleMode (fwd->grabWindow->screen))
		{
		    int distX, distY;

		    case ScaleModeToOppositeCorner:
			distX =  (fww->outputRect.x1 + (fww->outputRect.x2 - fww->outputRect.x1) / 2.0f) - (WIN_REAL_X (fwd->grabWindow) + WIN_REAL_W (fwd->grabWindow) / 2.0f);
			distY = (fww->outputRect.y1 + (fww->outputRect.y2 - fww->outputRect.y1) / 2.0f) - (WIN_REAL_Y (fwd->grabWindow) + WIN_REAL_H (fwd->grabWindow) / 2.0f);

			moveWindow(fwd->grabWindow, distX, distY, TRUE, TRUE);
			syncWindowPosition (fwd->grabWindow);

			FWCalculateInputOrigin (fwd->grabWindow, WIN_REAL_X (fwd->grabWindow) +
						WIN_REAL_W (fwd->grabWindow) / 2.0f,
						WIN_REAL_Y (fwd->grabWindow) +
						WIN_REAL_H (fwd->grabWindow) / 2.0f);
			FWCalculateOutputOrigin (fwd->grabWindow, WIN_OUTPUT_X (fwd->grabWindow) +
						 WIN_OUTPUT_W (fwd->grabWindow) / 2.0f,
						 WIN_OUTPUT_Y (fwd->grabWindow) +
						 WIN_OUTPUT_H (fwd->grabWindow) / 2.0f);

			break;
		     default:
			break;

		}

		removeScreenGrab(s, fws->grabIndex, 0);
		fws->grabIndex = 0;
		fwd->grabWindow = NULL;
		fww->grab = grabNone;
	    }
	}
    }

    action->state &= ~ (CompActionStateTermKey | CompActionStateTermButton);

    return FALSE;
}

#define GET_WINDOW \
    CompWindow *tW, *w; \
    CompScreen *s; \
    Window xid; \
    FWWindowInputInfo *info; \
    xid = getIntOptionNamed (option, nOption, "window", 0); \
    tW = findWindowAtDisplay (d, xid); \
    w = tW; \
    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "root", 0)); \
    if (s && w && tW) \
    { \
	FREEWINS_SCREEN(s); \
    for (info = fws->transformedWindows; info; info = info->next) \
    { \
        if (tW->id == info->ipw) \
        w = FWGetRealWindow (tW); \
        break; \
    } \
    } \


/* Repetitive Stuff */

void
FWSetPrepareRotation (CompWindow *w,
                      float dx,
                      float dy,
                      float dz,
                      float dsu,
                      float dsd)
{
    FREEWINS_WINDOW (w);

    if (matchEval (freewinsGetShapeWindowTypes (w->screen), w))
    {

	FWCalculateInputOrigin(w, WIN_REAL_X (w) +
				  WIN_REAL_W (w) / 2.0f,
	                          WIN_REAL_Y (w) +
	                          WIN_REAL_H (w) / 2.0f);
	FWCalculateOutputOrigin(w, WIN_OUTPUT_X (w) +
				   WIN_OUTPUT_W (w) / 2.0f,
	                           WIN_OUTPUT_Y (w) +
	                           WIN_OUTPUT_H (w) / 2.0f);

	fww->transform.unsnapAngX += dy;
	fww->transform.unsnapAngY -= dx;
	fww->transform.unsnapAngZ += dz;

	fww->transform.unsnapScaleX += dsu;
	fww->transform.unsnapScaleY += dsd;

	fww->animate.oldAngX = fww->transform.angX;
	fww->animate.oldAngY = fww->transform.angY;
	fww->animate.oldAngZ = fww->transform.angZ;

	fww->animate.oldScaleX = fww->transform.scaleX;
	fww->animate.oldScaleY = fww->transform.scaleY;

	fww->animate.destAngX = fww->transform.angX + dy;
	fww->animate.destAngY = fww->transform.angY - dx;
	fww->animate.destAngZ = fww->transform.angZ + dz;

	fww->animate.destScaleX = fww->transform.scaleX + dsu;
	fww->animate.destScaleY = fww->transform.scaleY + dsd;

    }
}

#define ROTATE_INC freewinsGetRotateIncrementAmount (w->screen)
#define NEG_ROTATE_INC freewinsGetRotateIncrementAmount (w->screen) *-1

#define SCALE_INC freewinsGetScaleIncrementAmount (w->screen)
#define NEG_SCALE_INC freewinsGetScaleIncrementAmount (w->screen) *-1

Bool
FWRotateUp (CompDisplay *d,
            CompAction *action,
            CompActionState state,
	    CompOption *option,
	    int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, ROTATE_INC, 0, 0, 0);

        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }

    return TRUE;

}

Bool
FWRotateDown (CompDisplay *d,
	      CompAction *action,
	      CompActionState state,
	      CompOption *option,
	      int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, NEG_ROTATE_INC, 0, 0, 0);

        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    return TRUE;

}

Bool
FWRotateLeft (CompDisplay *d,
	      CompAction *action,
	      CompActionState state,
	      CompOption *option,
	      int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, ROTATE_INC, 0, 0, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }

    return TRUE;

}

Bool
FWRotateRight (CompDisplay *d,
	       CompAction *action,
	       CompActionState state,
	       CompOption *option,
	       int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, NEG_ROTATE_INC, 0, 0, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }

    return TRUE;

}

Bool
FWRotateClockwise (CompDisplay *d,
		   CompAction *action,
		   CompActionState state,
		   CompOption *option,
		   int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, ROTATE_INC, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }

    return TRUE;

}

Bool
FWRotateCounterclockwise (CompDisplay *d,
			   CompAction *action,
			   CompActionState state,
			   CompOption *option,
			   int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, NEG_ROTATE_INC, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }

    return TRUE;

}

Bool
FWScaleUp (CompDisplay *d,
	   CompAction *action,
	   CompActionState state,
	   CompOption *option,
	   int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, 0, SCALE_INC, SCALE_INC);
        addWindowDamage (w); // Smoothen Painting
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);

        FREEWINS_WINDOW (w);

	if (!freewinsGetAllowNegative (w->screen))
	{
	    float minScale = freewinsGetMinScale (w->screen);
	    if (fww->animate.destScaleX < minScale)
		fww->animate.destScaleX = minScale;

	    if (fww->animate.destScaleY < minScale)
		fww->animate.destScaleY = minScale;
	}
    }

    return TRUE;

}

Bool
FWScaleDown (CompDisplay *d,
	       CompAction *action,
	       CompActionState state,
	       CompOption *option,
	       int nOption)
{

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, 0, NEG_SCALE_INC, NEG_SCALE_INC);
        addWindowDamage (w); // Smoothen Painting
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);


	FREEWINS_WINDOW (w);

        /* Stop scale at threshold specified */
	if (!freewinsGetAllowNegative (w->screen))
	{
	    float minScale = freewinsGetMinScale (w->screen);
	    if (fww->animate.destScaleX < minScale)
		fww->animate.destScaleX = minScale;

	    if (fww->animate.destScaleY < minScale)
		fww->animate.destScaleY = minScale;
	}
    }

    return TRUE;

}

/* Reset the Rotation and Scale to 0 and 1 */
Bool
resetFWTransform (CompDisplay *d,
		   CompAction *action,
		   CompActionState state,
		   CompOption *option,
		   int nOption)
{

    GET_WINDOW;
    if (w)
    {
        FREEWINS_WINDOW (w);
        FWSetPrepareRotation (w, fww->transform.angY,
                                 -fww->transform.angX,
                                 -fww->transform.angZ,
                                 (1 - fww->transform.scaleX),
                                 (1 - fww->transform.scaleY));
        addWindowDamage (w);

	fww->transformed = FALSE;

        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);

        fww->resetting = TRUE;
    }

    return TRUE;
}

/* Callable action to rotate a window to the angle provided
 * x: Set angle to x degrees
 * y: Set angle to y degrees
 * z: Set angle to z degrees
 * window: The window to apply the transformation to
 */
Bool
freewinsRotateWindow (CompDisplay *d,
                      CompAction *action,
                      CompActionState state,
                      CompOption *option,
                      int nOption)
{
    CompWindow *w;

    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));

    if (w)
    {
        FREEWINS_WINDOW(w);

        float x, y, z;

        y = getFloatOptionNamed(option, nOption, "x", 0.0f);
        x = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);

        FWSetPrepareRotation (w, x - fww->animate.destAngX,
				 y - fww->animate.destAngY,
				 z - fww->animate.destAngZ, 0, 0);
        addWindowDamage (w);

    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/* Callable action to increment window rotation by the angles provided
 * x: Increment angle by x degrees
 * y: Increment angle by y degrees
 * z: Increment angle by z degrees
 * window: The window to apply the transformation to
 */
Bool
freewinsIncrementRotateWindow (CompDisplay *d,
                              CompAction *action,
                              CompActionState state,
                              CompOption *option,
                              int nOption)
{
    CompWindow *w;

    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));

    if (w)
    {
        float x, y, z;

        x = getFloatOptionNamed(option, nOption, "x", 0.0f);
        y = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);

        /* Respect dx, dy, dz, first */
        FWSetPrepareRotation (w, x, y, z, 0, 0);
        addWindowDamage (w);
    }
    else
    {
        return FALSE;
    }

    if (FWCanShape (w))
        FWHandleWindowInputInfo (w);

    return TRUE;
}

/* Callable action to scale a window to the scale provided
 * x: Set scale to x factor
 * y: Set scale to y factor
 * window: The window to apply the transformation to
 */
Bool
freewinsScaleWindow (CompDisplay *d,
                      CompAction *action,
                      CompActionState state,
                      CompOption *option,
                      int nOption)
{
    CompWindow *w;
    float x, y;

    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));

    if (w)
    {
	FREEWINS_WINDOW (w);

        x = getFloatOptionNamed(option, nOption, "x", 0.0f);
        y = getFloatOptionNamed(option, nOption, "y", 0.0f);

        FWSetPrepareRotation (w, 0, 0, 0,
			      x - fww->animate.destScaleX,
			      y - fww->animate.destScaleY);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);

        /* Stop scale at threshold specified */
	if (!freewinsGetAllowNegative (w->screen))
	{
	    float minScale = freewinsGetMinScale (w->screen);
	    if (fww->animate.destScaleX < minScale)
		fww->animate.destScaleX = minScale;

	    if (fww->animate.destScaleY < minScale)
		fww->animate.destScaleY = minScale;
	}

        addWindowDamage (w);
    }
    else
    {
        return FALSE;
    }

    if (FWCanShape (w))
        FWHandleWindowInputInfo (w);

    return TRUE;
}

/* Toggle Axis-Help Display */
Bool
toggleFWAxis (CompDisplay *d,
              CompAction *action,
              CompActionState state,
              CompOption *option,
              int nOption)
{

    CompScreen *s;

    FREEWINS_DISPLAY(d);

    s = findScreenAtDisplay(d, getIntOptionNamed(option, nOption, "root", 0));

    fwd->axisHelp = !fwd->axisHelp;
    if (s)
        damageScreen (s);

    return TRUE;
}
