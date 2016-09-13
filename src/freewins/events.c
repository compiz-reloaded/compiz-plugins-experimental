/**
 * Compiz Fusion Freewins plugin
 *
 * events.c
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
 * -  X Input Redirection
 *  - Code could be cleaner
 *  - Add timestep and speed options to animation
 *  - Add window hover-over info via paintOutput : i.e
 *    - Resize borders
 *    - 'Reset' Button
 *    - 'Scale' Button
 *    - 'Rotate' Button
 */

#include "freewins.h"


/* ------ Event Handlers ------------------------------------------------*/

static void
FWHandleIPWResizeInitiate (CompWindow *w)
{
    FREEWINS_WINDOW (w);
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    (*w->screen->activateWindow) (w);
    fww->grab = grabResize;
    fws->rotateCursor = XCreateFontCursor (w->screen->display->display, XC_plus);
	if(!otherScreenGrabExist(w->screen, "freewins", "resize", 0))
	    if(!fws->grabIndex)
        {
        unsigned int mods = 0;
        mods &= CompNoMask;
		fws->grabIndex = pushScreenGrab(w->screen, fws->rotateCursor, "resize");
	    (w->screen->windowGrabNotify) (w,  w->attrib.x + (w->width / 2),
                                           w->attrib.y + (w->height / 2), mods,
					                       CompWindowGrabMoveMask |
					                       CompWindowGrabButtonMask);
        fwd->grabWindow = w;
        }
}

static void
FWHandleIPWMoveInitiate (CompWindow *w)
{
    FREEWINS_WINDOW (w);
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    (*w->screen->activateWindow) (w);
    fww->grab = grabMove;
    fws->rotateCursor = XCreateFontCursor (w->screen->display->display, XC_fleur);
	if(!otherScreenGrabExist(w->screen, "freewins", "move", 0))
	    if(!fws->grabIndex)
        {
        unsigned int mods = 0;
        mods &= CompNoMask;
		fws->grabIndex = pushScreenGrab(w->screen, fws->rotateCursor, "move");
	    (w->screen->windowGrabNotify) (w,  w->attrib.x + (w->width / 2),
                                           w->attrib.y + (w->height / 2), mods,
					                       CompWindowGrabMoveMask |
					                       CompWindowGrabButtonMask);
        }
    fwd->grabWindow = w;
}

static void
FWHandleIPWMoveMotionEvent (CompWindow *w, unsigned int x, unsigned int y)
{
    FREEWINS_SCREEN (w->screen);

    int dx = x - lastPointerX;
    int dy = y - lastPointerY;

    if (!fws->grabIndex)
        return;

    moveWindow(w, dx,
                  dy, TRUE, freewinsGetImmediateMoves (w->screen));
    syncWindowPosition (w);

}

static void FWHandleIPWResizeMotionEvent (CompWindow *w, unsigned int x, unsigned int y)
{
    FREEWINS_WINDOW (w);

    int dx = (x - lastPointerX) * 10;
    int dy = (y - lastPointerY) * 10;

    fww->winH += dx;
    fww->winW += dy;

    /* In order to prevent a window redraw on resize
     * on every motion event we have a threshold
     */

     /* FIXME: cf-love: Instead of actually resizing the window, scale it up, then resize it */

    if (fww->winH - 10 > w->height || fww->winW - 10 > w->width)
    {
        XWindowChanges xwc;
        unsigned int   mask = CWX | CWY | CWWidth | CWHeight;

        xwc.x = w->serverX;
        xwc.y = w->serverX;
        xwc.width = fww->winW;
        xwc.height = fww->winH;

        if (xwc.width == w->serverWidth)
	    mask &= ~CWWidth;

        if (xwc.height == w->serverHeight)
	    mask &= ~CWHeight;

        if (w->mapNum && (mask & (CWWidth | CWHeight)))
	    sendSyncRequest (w);

        configureXWindow (w, mask, &xwc);
    }

}


/* Handle Rotation */
static void FWHandleRotateMotionEvent (CompWindow *w, float dx, float dy, int x, int y)
{
    FREEWINS_WINDOW (w);
    FREEWINS_DISPLAY (w->screen->display);

    x -= 100;
    y -= 100;

    int oldX = lastPointerX - 100;
    int oldY = lastPointerY - 100;

    float midX = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0;
    float midY = WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0;

    float angX;
    float angY;
    float angZ;

    /* Save the current angles so we can work with them */
    if (freewinsGetSnap (w->screen) || fwd->snap)
    {
         angX = fww->transform.unsnapAngX;
         angY = fww->transform.unsnapAngY;
         angZ = fww->transform.unsnapAngZ;
    }
    else
    {
         angX = fww->animate.destAngX;
         angY = fww->animate.destAngY;
         angZ = fww->animate.destAngZ;
    }

  /* Check for Y axis clicking (Top / Bottom) */
    if (pointerY > midY)
    {
	/* Check for X axis clicking (Left / Right) */
	if (pointerX > midX)
	    fww->corner = CornerBottomRight;
	else if (pointerX < midX)
	    fww->corner = CornerBottomLeft;
    }
    else if (pointerY < midY)
    {
	/* Check for X axis clicking (Left / Right) */
	if (pointerX > midX)
	    fww->corner = CornerTopRight;
	else if (pointerX < midX)
	    fww->corner = CornerTopLeft;
    }

    float percentFromXAxis = 0.0, percentFromYAxis = 0.0;

    if (freewinsGetZAxisRotation (w->screen) == ZAxisRotationInterchangable)
    {

        /* Trackball rotation was too hard to implement. If anyone can implement it,
         * please come forward so I can replace this hacky solution to the problem.
         * Anyways, what happens here, is that we determine how far away we are from
         * each axis (y and x). The further we are away from the y axis, the more
         * up / down movements become Z axis movements and the further we are away from
         * the x-axis, the more left / right movements become z rotations. */

        /* We determine this by taking a percentage of how far away the cursor is from
         * each axis. We divide the 3D rotation by this percentage ( and divide by the
         * percentage squared in order to ensure that rotation is not too violent when we
         * are quite close to the origin. We multiply the 2D rotation by this percentage also
         * so we are essentially rotating in 3D and 2D all the time, but this is only really
         * noticeable when you move the cursor over to the extremes of a window. In every case
         * percentage can be defined as decimal-percentage (i.e 0.036 == 3.6%). Like I mentioned
         * earlier, if you can replace this with trackball rotation, please come forward! */

        float halfWidth = WIN_REAL_W (w) / 2.0f;
        float halfHeight = WIN_REAL_H (w) / 2.0f;

        float distFromXAxis = fabs (fww->iMidX - pointerX);
        float distFromYAxis = fabs (fww->iMidY - pointerY);

        percentFromXAxis = distFromXAxis / halfWidth;
        percentFromYAxis = distFromYAxis / halfHeight;

    }
    else if (freewinsGetZAxisRotation (w->screen) == ZAxisRotationSwitch)
        FWDetermineZAxisClick (w, pointerX, pointerY, TRUE);

    dx *= 360;
    dy *= 360;

    /* Handle inversion */

    Bool can2D = fww->can2D, can3D = fww->can3D;

    if (fwd->invert && freewinsGetZAxisRotation (w->screen) != ZAxisRotationInterchangable)
    {
        can2D = !fww->can2D;
        can3D = !fww->can3D;
    }

    if(can2D)
    {

       float zX = 1.0f;
       float zY = 1.0f;

       if (freewinsGetZAxisRotation (w->screen) == ZAxisRotationInterchangable)
       {
            zX = percentFromXAxis;
            zY = percentFromYAxis;
       }

       zX = zX > 1.0f ? 1.0f : zX;
       zY = zY > 1.0f ? 1.0f : zY;

       switch (fww->corner)
       {
            case CornerTopRight:

                if ((x) < oldX)
                angZ -= dx * zX;
                else if ((x) > oldX)
                angZ += dx * zX;


                if ((y) < oldY)
                angZ -= dy * zY;
                else if ((y) > oldY)
                angZ += dy * zY;

                break;

            case CornerTopLeft:

                if ((x) < oldX)
                angZ -= dx * zX;
                else if ((x) > oldX)
                angZ += dx * zX;


                if ((y) < oldY)
                angZ += dy * zY;
                else if ((y) > oldY)
                angZ -= dy * zY;

                break;

            case CornerBottomLeft:

                if ((x) < oldX)
                angZ += dx * zX;
                else if ((x) > oldX)
                angZ -= dx * zX;


                if ((y) < oldY)
                angZ += dy * zY;
                else if ((y) > oldY)
                angZ -= dy * zY;

                break;

            case CornerBottomRight:

                if ((x) < oldX)
                angZ += dx * zX;
                else if ((x) > oldX)
                angZ -= dx * zX;


                if ((y) < oldY)
                angZ -= dy * zY;
                else if ((y) > oldY)
                angZ += dy * zY;
                break;
        }
    }

    if (can3D)
    {
        if (freewinsGetZAxisRotation (w->screen) != ZAxisRotationInterchangable)
        {
            percentFromXAxis = 0.0f;
            percentFromYAxis = 0.0f;
        }


    angX -= dy * (1 - percentFromXAxis);
    angY += dx * (1 - percentFromYAxis);
    }

    /* Restore angles */

    if (freewinsGetSnap (w->screen) || fwd->snap)
    {
         fww->transform.unsnapAngX = angX;
         fww->transform.unsnapAngY = angY;
         fww->transform.unsnapAngZ = angZ;
    }
    else
    {
         fww->animate.destAngX = angX;
         fww->animate.destAngY = angY;
         fww->animate.destAngZ = angZ;
    }

    FWHandleSnap(w);
}

/* Handle Scaling */
static void FWHandleScaleMotionEvent (CompWindow *w, float dx, float dy, int x, int y)
{
    FREEWINS_WINDOW (w);
    FREEWINS_DISPLAY (w->screen->display);

    x -= 100.0;
    y -= 100.0;

    int oldX = lastPointerX - 100;
    int oldY = lastPointerY - 100;

    float scaleX, scaleY;

    if (freewinsGetSnap (w->screen) || fwd->snap)
    {
        scaleX = fww->transform.unsnapScaleX;
        scaleY = fww->transform.unsnapScaleY;
    }
    else
    {
        scaleX = fww->animate.destScaleX;
        scaleY = fww->animate.destScaleY;
    }

    FWCalculateInputRect (w);

    switch (fww->corner)
    {

        case CornerTopLeft:

        if ((x) < oldX)
        scaleX -= dx;
        else if ((x) > oldX)
        scaleX -= dx;

        if ((y) < oldY)
        scaleY -= dy;
        else if ((y) > oldY)
        scaleY -= dy;
        break;

        case CornerTopRight:

        if ((x) < oldX)
        scaleX += dx;
        else if ((y) > oldX)
        scaleX += dx;


        // Check Y Direction
        if ((y) < oldY)
        scaleY -= dy;
        else if ((y) > oldY)
        scaleY -= dy;

        break;

        case CornerBottomLeft:

        // Check X Direction
        if ((x) < oldX)
        scaleX -= dx;
        else if ((y) > oldX)
        scaleX -= dx;

        // Check Y Direction
        if ((y) < oldY)
        scaleY += dy;
        else if ((y) > oldY)
        scaleY += dy;

        break;

        case CornerBottomRight:

        // Check X Direction
        if ((x) < oldX)
        scaleX += dx;
        else if ((x) > oldX)
        scaleX += dx;

        // Check Y Direction
        if ((y) < oldY)
        scaleY += dy;
        else if ((y) > oldY)
        scaleY += dy;
        break;
    }

    if (freewinsGetSnap (w->screen) || fwd->snap)
    {
       fww->transform.unsnapScaleX = scaleX;
        fww->transform.unsnapScaleY = scaleY;
    }
    else
    {
        fww->animate.destScaleX = scaleX;
        fww->animate.destScaleY = scaleY;
    }

    /* Stop scale at threshold specified */
    if (!freewinsGetAllowNegative (w->screen))
    {
        float minScale = freewinsGetMinScale (w->screen);
        if (fww->animate.destScaleX < minScale)
          fww->animate.destScaleX = minScale;

        if (fww->animate.destScaleY < minScale)
          fww->animate.destScaleY = minScale;
    }

    /* Change scales for maintaining aspect ratio */
    if (freewinsGetScaleUniform (w->screen))
    {
        float tempscaleX = fww->animate.destScaleX;
        float tempscaleY = fww->animate.destScaleY;
        fww->animate.destScaleX = (tempscaleX + tempscaleY) / 2;
        fww->animate.destScaleY = (tempscaleX + tempscaleY) / 2;
        fww->transform.unsnapScaleX = (tempscaleX + tempscaleY) / 2;
        fww->transform.unsnapScaleY = (tempscaleX + tempscaleY) / 2;
    }

    FWHandleSnap(w);
}

static void
FWHandleButtonReleaseEvent (CompWindow *w)
{
	FREEWINS_WINDOW (w);
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    if (fww->grab == grabMove || fww->grab == grabResize)
    {
        removeScreenGrab (w->screen, fws->grabIndex, NULL);
        (w->screen->windowUngrabNotify) (w);
        moveInputFocusToWindow (w);
		FWAdjustIPW (w);
        fws->grabIndex = 0;
        fwd->grabWindow = NULL;
        fww->grab = grabNone;
    }
}

static void
FWHandleEnterNotify (CompWindow *w,
                     XEvent *xev)
{
    XEvent EnterNotifyEvent;

    memcpy (&EnterNotifyEvent.xcrossing, &xev->xcrossing,
	    sizeof (XCrossingEvent));

    if (w)
    {
	EnterNotifyEvent.xcrossing.window = w->id;
	XSendEvent (w->screen->display->display, w->id,
		    FALSE, EnterWindowMask, &EnterNotifyEvent);
    }
}

static void
FWHandleLeaveNotify (CompWindow *w,
                     XEvent *xev)
{
    XEvent LeaveNotifyEvent;

    memcpy (&LeaveNotifyEvent.xcrossing, &xev->xcrossing,
            sizeof (XCrossingEvent));
    LeaveNotifyEvent.xcrossing.window = w->id;

    XSendEvent (w->screen->display->display, w->id, FALSE,
                LeaveWindowMask, &LeaveNotifyEvent);
}


/* X Event Handler */
void FWHandleEvent(CompDisplay *d, XEvent *ev){

    float dx, dy;
    CompWindow *oldPrev, *oldNext, *w;
    FREEWINS_DISPLAY(d);

    w = oldPrev = oldNext = NULL;

    /* Check our modifiers first */

    if (ev->type == d->xkbEvent)
    {
	XkbAnyEvent *xkbEvent = (XkbAnyEvent *) ev;

	if (xkbEvent->xkb_type == XkbStateNotify)
	{
	    XkbStateNotifyEvent *stateEvent = (XkbStateNotifyEvent *) ev;
	    unsigned int snapMods = 0xffffffff;
	    unsigned int invertMods = 0xffffffff;

	    if (fwd->snapMask)
		snapMods = fwd->snapMask;

	    if ((stateEvent->mods & snapMods) == snapMods)
		fwd->snap = TRUE;
	    else
		fwd->snap = FALSE;

	    if (fwd->invertMask)
		invertMods = fwd->invertMask;

	    if ((stateEvent->mods & invertMods) == invertMods)
		fwd->invert = TRUE;
	    else
		fwd->invert = FALSE;

	}
    }

    switch(ev->type){

    case EnterNotify:
    {
        CompWindow *btnW;
        btnW = findWindowAtDisplay (d, ev->xbutton.subwindow);

        /* It wasn't the subwindow, try the window */
        if (!btnW)
            btnW = findWindowAtDisplay (d, ev->xbutton.window);

        /* We have established the CompWindow we clicked
         * on. Get the real window */
        if (btnW)
        {
            if (FWCanShape (btnW) && !fwd->grabWindow && !otherScreenGrabExist(btnW->screen, 0))
                fwd->hoverWindow = btnW;
            btnW = FWGetRealWindow (btnW);
        }

        if (btnW)
        {
            if (FWCanShape (btnW) && !fwd->grabWindow && !otherScreenGrabExist(btnW->screen, 0))
                fwd->hoverWindow = btnW;
            FWHandleEnterNotify (btnW, ev);
        }
    }
    break;
    case LeaveNotify:
    {

        CompWindow *btnW;
        btnW = findWindowAtDisplay (d, ev->xbutton.subwindow);

        /* It wasn't the subwindow, try the window */
        if (!btnW)
            btnW = findWindowAtDisplay (d, ev->xbutton.window);

        /* We have established the CompWindow we clicked
         * on. Get the real window */
        if (btnW)
            btnW = FWGetRealWindow (btnW);

        if (btnW)
            FWHandleLeaveNotify (btnW, ev);
    }
    break;
    case MotionNotify:

    if(fwd->grabWindow)
    {
	FREEWINS_WINDOW(fwd->grabWindow);

	dx = ((float)(pointerX - lastPointerX) / fwd->grabWindow->screen->width) * \
            freewinsGetMouseSensitivity (fwd->grabWindow->screen);
	dy = ((float)(pointerY - lastPointerY) / fwd->grabWindow->screen->height) * \
            freewinsGetMouseSensitivity (fwd->grabWindow->screen);

	if (matchEval (freewinsGetShapeWindowTypes (fwd->grabWindow->screen), fwd->grabWindow))
	{
	    if (fww->grab == grabMove || fww->grab == grabResize)
	    {
		FREEWINS_SCREEN (fwd->grabWindow->screen);
		FWWindowInputInfo *info;
		CompWindow *w = fwd->grabWindow;
		for (info = fws->transformedWindows; info; info = info->next)
		{
		    if (fwd->grabWindow->id == info->ipw)
		    /* The window we just grabbed was actually
		     * an IPW, get the real window instead
		      */
			w = FWGetRealWindow (fwd->grabWindow);
		}
		switch (fww->grab)
		{
		    case grabMove:
			FWHandleIPWMoveMotionEvent (w, pointerX, pointerY); break;
		    case grabResize:
		        FWHandleIPWResizeMotionEvent (w, pointerX, pointerY); break;
		    default:
		        break;
		}
	    }
	}

	if (fww->grab == grabRotate)
	{
	    FWHandleRotateMotionEvent(fwd->grabWindow, dx, dy, ev->xmotion.x, ev->xmotion.y);
	}
	if (fww->grab == grabScale)
	{
	    FWHandleScaleMotionEvent(fwd->grabWindow, dx * 3, dy * 3, ev->xmotion.x, ev->xmotion.y);
	}

	if(dx != 0.0 || dy != 0.0)
	    FWDamageArea (fwd->grabWindow);
    }
    break;


    /* Button Press and Release */
    case ButtonPress:
    {
        CompWindow *btnW;
        btnW = findWindowAtDisplay (d, ev->xbutton.subwindow);

        /* It wasn't the subwindow, try the window */
        if (!btnW)
            btnW = findWindowAtDisplay (d, ev->xbutton.window);

        /* We have established the CompWindow we clicked
         * on. Get the real window
         * FIXME: Free btnW and use another CompWindow * such as realW
         */
        if (btnW)
        btnW = FWGetRealWindow (btnW);

        if (btnW)
        {
	    if (matchEval (freewinsGetShapeWindowTypes (btnW->screen), btnW))
                switch (ev->xbutton.button)
                {
                    case Button1:
                        FWHandleIPWMoveInitiate (btnW); break;
                    case Button3:
                        FWHandleIPWResizeInitiate (btnW); break;
                    default:
                        break;
                }
        }

        fwd->click_root_x = ev->xbutton.x_root;
        fwd->click_root_y = ev->xbutton.y_root;

    }
    break;

    case ButtonRelease:
    {
        if (fwd->grabWindow)
        {
	    FREEWINS_WINDOW (fwd->grabWindow);

	    if (matchEval (freewinsGetShapeWindowTypes (fwd->grabWindow->screen), fwd->grabWindow))
            if (fww->grab == grabMove || fww->grab == grabResize)
            {
                FWHandleButtonReleaseEvent (fwd->grabWindow);
		        fwd->grabWindow = 0;
            }
	}
    }
    break;

    case ConfigureNotify:
	w = findWindowAtDisplay (d, ev->xconfigure.window);
	if (w)
	{
	    oldPrev = w->prev;
	    oldNext = w->next;

            FREEWINS_WINDOW (w);

            fww->winH = WIN_REAL_H (w);
            fww->winW = WIN_REAL_W (w);
	}
    break;

    case ClientMessage:
    if (ev->xclient.message_type == d->desktopViewportAtom)
    {
        /* Viewport change occurred, or something like that - adjust the IPW's */
        CompScreen *s;
        CompWindow *adjW, *actualW;

        for (s = d->screens; s; s = s->next)
            for (adjW = s->windows; adjW; adjW = adjW->next)
            {
                int vX = 0, vY = 0, dX, dY;

                actualW = FWGetRealWindow (adjW);

                if (!actualW)
                    actualW = adjW;

                if (actualW)
                {
                    CompWindow *ipw;

                    FREEWINS_WINDOW (actualW);

                    if (!fww->input || fww->input->ipw)
                        break;

		    ipw = findWindowAtDisplay (d, fww->input->ipw);
	            if (ipw)
	            {
			dX = s->x - vX;
			dY = s->y - vY;

			defaultViewportForWindow (actualW, &vX, &vY);
			moveWindowToViewportPosition (ipw,
		                          ipw->attrib.x - dX * s->width,
		                          ipw->attrib.y - dY * s->height,
		                          FALSE);
	            }
                }
            }
    }
    break;

    default:
    if (ev->type == d->shapeEvent + ShapeNotify)
    {
        XShapeEvent *se = (XShapeEvent *) ev;
        if (se->kind == ShapeInput)
        {
	        CompWindow *w;
	        w = findWindowAtDisplay (d, se->window);
	        if (w)
	        {
	            FREEWINS_WINDOW (w);

	            if (FWCanShape (w) && (fww->transform.scaleX != 1.0f || fww->transform.scaleY != 1.0f))
	            {
	                // Reset the window back to normal
	                fww->transform.scaleX = 1.0f;
	                fww->transform.scaleY = 1.0f;
                fww->transform.angX = 0.0f;
                fww->transform.angY = 0.0f;
                fww->transform.angZ = 0.0f;
		            /*FWShapeInput (w); - Disabled due to problems it causes*/
		        }
	        }
        }
    }
    }

    UNWRAP(fwd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(fwd, d, handleEvent, FWHandleEvent);

    /* Now we can find out if a restacking occurred while we were handing events */

    switch (ev->type)
    {
	case ConfigureNotify:
	    if (w) /* already assigned above */
	    {
		if (w->prev != oldPrev || w->next != oldNext)
		{
		    /* restacking occured, ensure ipw stacking */
		    FWAdjustIPWStacking (w->screen);
		    FWAdjustIPW (w);
		}
	    }
	break;
    }
}
