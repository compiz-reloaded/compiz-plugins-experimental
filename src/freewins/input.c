/**
 * Compiz Fusion Freewins plugin
 *
 * input.c
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

#include "freewins.h"
#include <cairo/cairo-xlib.h>

/* ------ Input Prevention -------------------------------------------*/

/* Shape the IPW
  * Thanks to Joel Bosveld (b0le)
  * for helping me with this section
  */
static void
FWShapeIPW (CompWindow *w)
{

	FREEWINS_WINDOW (w);

	if (fww->input)
	{

	Window xipw = fww->input->ipw;
	CompWindow *ipw = findWindowAtDisplay (w->screen->display, xipw);

	if (ipw)
	{
    cairo_t 				*cr;
    int               width, height;

    width = fww->inputRect.x2 - fww->inputRect.x1;
    height = fww->inputRect.y2 - fww->inputRect.y1;

    Pixmap b = XCreatePixmap (ipw->screen->display->display, xipw, width, height, 1);

    cairo_surface_t *bitmap =
    cairo_xlib_surface_create_for_bitmap (ipw->screen->display->display,
					 b,
					 DefaultScreenOfDisplay (ipw->screen->display->display),
					 width, height);

    cr = cairo_create (bitmap);

    cairo_save (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_restore (cr);

    /* Move to our first corner (TopLeft)  */

    cairo_move_to (cr,
                   fww->output.shapex1 - MIN(fww->inputRect.x1, fww->inputRect.x2),
                   fww->output.shapey1 - MIN(fww->inputRect.y1, fww->inputRect.y2));

    /* Line to TopRight */

    cairo_line_to (cr,
                   fww->output.shapex2 - MIN(fww->inputRect.x1, fww->inputRect.x2),
                   fww->output.shapey2 - MIN(fww->inputRect.y1, fww->inputRect.y2));

    /* Line to BottomRight */

    cairo_line_to (cr,
                   fww->output.shapex4 - MIN(fww->inputRect.x1, fww->inputRect.x2),
                   fww->output.shapey4 - MIN(fww->inputRect.y1, fww->inputRect.y2));

    /* Line to BottomLeft */

    cairo_line_to (cr,
                   fww->output.shapex3 - MIN(fww->inputRect.x1, fww->inputRect.x2),
                   fww->output.shapey3 - MIN(fww->inputRect.y1, fww->inputRect.y2));

    /* Line to TopLeft*/

    cairo_line_to (cr,
                   fww->output.shapex1 - MIN(fww->inputRect.x1, fww->inputRect.x2),
                   fww->output.shapey1 - MIN(fww->inputRect.y1, fww->inputRect.y2));

    /* Ensure it's all closed up */

    cairo_close_path (cr);

    /* Fill in the box */

    cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);
    cairo_fill (cr);

    /* This takes the bitmap we just drew with cairo
      * and scans out the white bits (You can see these)
      * if you uncomment the following line after this
      * comment. Then, all the bits we drew on are clickable,
      * leaving us with a nice and neat window shape. Yummy.
      */

     /* XWriteBitmapFile (ipw->screen->display->display,
							 "/path/to/your/image.bmp",
							 b,
							 ipw->width,
							 ipw->height,
							 -1, -1);  */

    XShapeCombineMask (ipw->screen->display->display, xipw,
					   ShapeBounding,
					   0,
					   0,
					   b,
					   ShapeSet);

    XFreePixmap (ipw->screen->display->display, b);
    cairo_surface_destroy (bitmap);
    cairo_destroy (cr);
    }
    }
}

static void
FWSaveInputShape (CompWindow *w,
	             XRectangle **retRects,
	             int        *retCount,
	             int        *retOrdering)
{
    XRectangle *rects;
    int        count = 0, ordering;
    Display    *dpy = w->screen->display->display;

    rects = XShapeGetRectangles (dpy, w->id, ShapeInput, &count, &ordering);

    /* check if the returned shape exactly matches the window shape -
       if that is true, the window currently has no set input shape */
    if ((count == 1) &&
	(rects[0].x == -w->serverBorderWidth) &&
	(rects[0].y == -w->serverBorderWidth) &&
	(rects[0].width == (w->serverWidth + w->serverBorderWidth)) &&
	(rects[0].height == (w->serverHeight + w->serverBorderWidth)))
    {
	count = 0;
    }

    *retRects    = rects;
    *retCount    = count;
    *retOrdering = ordering;
}

static void
FWUnshapeInput (CompWindow *w)
{
    Display *dpy = w->screen->display->display;

    FREEWINS_WINDOW (w);

    if (fww->input->nInputRects)
    {
	XShapeCombineRectangles (dpy, w->id, ShapeInput, 0, 0,
				 fww->input->inputRects, fww->input->nInputRects,
				 ShapeSet, fww->input->inputRectOrdering);
    }
    else
    {
	XShapeCombineMask (dpy, w->id, ShapeInput, 0, 0, None, ShapeSet);
    }

    if (fww->input->frameNInputRects >= 0)
    {
	if (fww->input->frameInputRects)
	{
	    XShapeCombineRectangles (dpy, w->frame, ShapeInput, 0, 0,
				     fww->input->frameInputRects,
				     fww->input->frameNInputRects,
				     ShapeSet,
				     fww->input->frameInputRectOrdering);
	}
	else
	{
	    XShapeCombineMask (dpy, w->frame, ShapeInput, 0, 0, None, ShapeSet);
	}
    }
}


/* Input Shaper. This no longer adjusts the shape of the window
    but instead shapes it to 0 as the IPW deals with the input.
  */
static void
FWShapeInput (CompWindow *w)
{
    CompWindow *fw;
    Display    *dpy = w->screen->display->display;

    FREEWINS_WINDOW (w);

    /* save old shape */
    FWSaveInputShape (w, &fww->input->inputRects,
			 &fww->input->nInputRects, &fww->input->inputRectOrdering);

    fw = findWindowAtDisplay (w->screen->display, w->frame);
    if (fw)
    {
	FWSaveInputShape(fw, &fww->input->frameInputRects,
			    &fww->input->frameNInputRects,
			    &fww->input->frameInputRectOrdering);
    }
    else
    {
	fww->input->frameInputRects        = NULL;
	fww->input->frameNInputRects       = -1;
	fww->input->frameInputRectOrdering = 0;
    }

    /* clear shape */
    XShapeSelectInput (dpy, w->id, NoEventMask);
    XShapeCombineRectangles  (dpy, w->id, ShapeInput, 0, 0,
			      NULL, 0, ShapeSet, 0);

    if (w->frame)
	XShapeCombineRectangles  (dpy, w->frame, ShapeInput, 0, 0,
				  NULL, 0, ShapeSet, 0);

    XShapeSelectInput (dpy, w->id, ShapeNotify);
}

/* Add the input info to the list of input info */
static void
FWAddWindowToList (FWWindowInputInfo *info)
{
    CompScreen        *s = info->w->screen;
    FWWindowInputInfo *run;

    FREEWINS_SCREEN (s);

    run = fws->transformedWindows;
    if (!run)
	fws->transformedWindows = info;
    else
    {
	for (; run->next; run = run->next);
	run->next = info;
    }
}

/* Remove the input info from the list of input info */
static void
FWRemoveWindowFromList (FWWindowInputInfo *info)
{
    CompScreen        *s = info->w->screen;
    FWWindowInputInfo *run;

    FREEWINS_SCREEN (s);

    if (!fws->transformedWindows)
	return;

    if (fws->transformedWindows == info)
	fws->transformedWindows = info->next;
    else
    {
	for (run = fws->transformedWindows; run->next; run = run->next)
	{
	    if (run->next == info)
	    {
		run->next = info->next;
		break;
	    }
	}
    }
}

/* Adjust size and location of the input prevention window
  */
void
FWAdjustIPW (CompWindow *w)
{
    XWindowChanges xwc;
    Display        *dpy = w->screen->display->display;
    float          width, height;

    FREEWINS_WINDOW (w);

    if (!fww->input || !fww->input->ipw)
	return;

	/* Repaint the window and recalculate rects */

    width  = fww->inputRect.x2 - fww->inputRect.x1;
    height = fww->inputRect.y2 - fww->inputRect.y1;

    xwc.x          = (fww->inputRect.x1);
    xwc.y          = (fww->inputRect.y1);
    xwc.width      = ceil(width);
    xwc.height     = ceil(height);
    xwc.stack_mode = Below;
    xwc.sibling    = w->id;

    XConfigureWindow (dpy, fww->input->ipw,
		      CWSibling | CWStackMode | CWX | CWY | CWWidth | CWHeight,
		      &xwc);

    XMapWindow (dpy, fww->input->ipw);

    /* Don't shape if we're grabbed */

    if (fww->grab == grabNone)
    FWShapeIPW (w);

}

/* Create an input prevention window */
static void
FWCreateIPW (CompWindow *w)
{
    Window               ipw;
    XSetWindowAttributes attrib;

    FREEWINS_WINDOW (w);

    if (!fww->input || fww->input->ipw)
	return;

    attrib.override_redirect = TRUE;
    attrib.event_mask        = 0;

    ipw = XCreateWindow (w->screen->display->display,
			 w->screen->root,
			 w->serverX - w->input.left,
			 w->serverY - w->input.top,
			 w->serverWidth + w->input.left + w->input.right,
			 w->serverHeight + w->input.top + w->input.bottom,
			 0, CopyFromParent, InputOnly, CopyFromParent,
			 CWEventMask | CWOverrideRedirect,
			 &attrib);

    fww->input->ipw = ipw;

    FWAdjustIPW (w);
}

/* Ensure that the input prevention window
 * is always on top of the transformed
 * window
 */

void
FWAdjustIPWStacking (CompScreen *s)
{
    FWWindowInputInfo *input;

    FREEWINS_SCREEN (s);

    for (input = fws->transformedWindows; input; input = input->next)
    {
	if (!input->w->prev || input->w->prev->id != input->ipw)
	    FWAdjustIPW (input->w);
    }
}

/* This should be called instead of FWShapeInput or FWCreateIPW
 * as it does all the setup beforehand.
 */
Bool
FWHandleWindowInputInfo (CompWindow *w)
{
    FREEWINS_WINDOW (w);

    if (!fww->transformed && fww->input)
    {
	if (fww->input->ipw)
	    XDestroyWindow (w->screen->display->display, fww->input->ipw);

	FWUnshapeInput (w);
	FWRemoveWindowFromList (fww->input);

	free (fww->input);
	fww->input = NULL;

	return FALSE;
    }
    else if (fww->transformed && !fww->input)
    {
	fww->input = calloc (1, sizeof (FWWindowInputInfo));
	if (!fww->input)
	    return FALSE;

	fww->input->w = w;
	FWShapeInput (w);
	FWCreateIPW (w);
	FWAddWindowToList (fww->input);
    }

    return TRUE;
}
