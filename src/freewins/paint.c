/**
 * Compiz Fusion Freewins plugin
 *
 * paint.c
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
 *        : Danny Baumann <dannybaumann@web.de>
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

/* ------ Window and Output Painting ------------------------------------*/

/* Damage util function */

void
FWDamageArea (CompWindow *w)
{
    FREEWINS_WINDOW (w);

    REGION region;

    region.rects = &region.extents;
    region.numRects = region.size = 1;

    region.extents.x1 = fww->outputRect.x1;
    region.extents.x2 = fww->outputRect.x2;
    region.extents.y1 = fww->outputRect.y1;
    region.extents.y2 = fww->outputRect.y2;

    damageScreenRegion (w->screen, &region);
}

/* Animation Prep */
void
FWPreparePaintScreen (CompScreen *s,
			            int	      ms)
{
    CompWindow *w;
    FREEWINS_SCREEN (s);

    /* FIXME: should only loop over all windows if at least one animation
       is running */
    for (w = s->windows; w; w = w->next)
    {
        FREEWINS_WINDOW (w);
        float speed = freewinsGetSpeed (s);
        fww->animate.steps = ((float) ms / ((20.1 - speed) * 100));

        if (fww->animate.steps < 0.005)
            fww->animate.steps = 0.005;

    /* Animation. We calculate how much increment
     * a window must rotate / scale per paint by
     * using the set destination attributes minus
     * the old attributes divided by the time
     * remaining.
     */

	/* Don't animate if the window is saved */


        fww->transform.angX += (float) fww->animate.steps * (fww->animate.destAngX - fww->transform.angX) * speed;
        fww->transform.angY += (float) fww->animate.steps * (fww->animate.destAngY - fww->transform.angY) * speed;
        fww->transform.angZ += (float) fww->animate.steps * (fww->animate.destAngZ - fww->transform.angZ) * speed;

        fww->transform.scaleX += (float) fww->animate.steps * (fww->animate.destScaleX - fww->transform.scaleX) * speed;
        fww->transform.scaleY += (float) fww->animate.steps * (fww->animate.destScaleY - fww->transform.scaleY) * speed;

        if (((fww->transform.angX >= fww->animate.destAngX - 0.05 &&
              fww->transform.angX <= fww->animate.destAngX + 0.05 ) &&
             (fww->transform.angY >= fww->animate.destAngY - 0.05 &&
              fww->transform.angY <= fww->animate.destAngY + 0.05 ) &&
             (fww->transform.angZ >= fww->animate.destAngZ - 0.05 &&
              fww->transform.angZ <= fww->animate.destAngZ + 0.05 ) &&
             (fww->transform.scaleX >= fww->animate.destScaleX - 0.00005 &&
              fww->transform.scaleX <= fww->animate.destScaleX + 0.00005 ) &&
             (fww->transform.scaleY >= fww->animate.destScaleY - 0.00005 &&
              fww->transform.scaleY <= fww->animate.destScaleY + 0.00005 )))
        {
            fww->resetting = FALSE;

            fww->transform.angX = fww->animate.destAngX;
            fww->transform.angY = fww->animate.destAngY;
            fww->transform.angZ = fww->animate.destAngZ;
            fww->transform.scaleX = fww->animate.destScaleX;
            fww->transform.scaleY = fww->animate.destScaleY;

            fww->transform.unsnapAngX = fww->animate.destAngX;
            fww->transform.unsnapAngY = fww->animate.destAngY;
            fww->transform.unsnapAngZ = fww->animate.destAngZ;
            fww->transform.unsnapScaleX = fww->animate.destScaleX;
            fww->transform.unsnapScaleY = fww->animate.destScaleX;
        }
        else
            FWDamageArea (w);
    }


    UNWRAP (fws, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (fws, s, preparePaintScreen, FWPreparePaintScreen);
}

/* Paint the window rotated or scaled */
Bool
FWPaintWindow (CompWindow *w,
               const WindowPaintAttrib *attrib,
               const CompTransform *transform,
               Region region,
               unsigned int mask)
{

    CompTransform wTransform = *transform;
    int currentCull, invertCull;
    glGetIntegerv (GL_CULL_FACE_MODE, &currentCull);
    invertCull = (currentCull == GL_BACK) ? GL_FRONT : GL_BACK;

    Bool status;

    FREEWINS_SCREEN (w->screen);
    FREEWINS_WINDOW (w);

    /* Has something happened? */

    /* Check to see if we are painting on a transformed screen*/
    /* Enable this code when we can animate between the two states */

    if ((fww->transform.angX != 0.0 ||
           fww->transform.angY != 0.0 ||
           fww->transform.angZ != 0.0 ||
           fww->transform.scaleX != 1.0 ||
           fww->transform.scaleY != 1.0 ||
           fww->oldWinX != WIN_REAL_X (w) ||
           fww->oldWinY != WIN_REAL_Y (w)) && matchEval (freewinsGetShapeWindowTypes (w->screen), w))
    {

	fww->oldWinX = WIN_REAL_X (w);
	fww->oldWinY = WIN_REAL_Y (w);

	/* Figure out where our 'origin' is, don't allow the origin to
	 * be where we clicked if the window is not grabbed, etc
	 */

        /* Here we duplicate some of the work the openGL does
         * but for different reasons. We have access to the
         * window's transformation matrix, so we will create
         * our own matrix and apply the same transformations
         * to it. From there, we create vectors for each point
         * that we wish to track and multiply them by this
         * matrix to give us the rotated / scaled co-ordinates.
         * From there, we project these co-ordinates onto the flat
         * screen that we have using the OGL viewport, projection
         * matrix and model matrix. Projection gives us three
         * co-ordinates, but we ignore Z and just use X and Y
         * to store in a surrounding rectangle. We can use this
         * surrounding rectangle to make things like shaping and
         * damage a lot more accurate than they used to be.
         */

         FWCalculateOutputRect (w);

         /* Prepare for transformation by doing
          * any neccesary adjustments
          */

         float autoScaleX = 1.0f;
         float autoScaleY = 1.0f;

         if (freewinsGetAutoZoom (w->screen))
         {
             float apparantWidth = fww->outputRect.x2 - fww->outputRect.x1;
             float apparantHeight = fww->outputRect.y2 - fww->outputRect.y1;

             autoScaleX = (float) WIN_OUTPUT_W (w) / (float) apparantWidth;
             autoScaleY = (float) WIN_OUTPUT_H (w) / (float) apparantHeight;

             if (autoScaleX >= 1.0f)
                autoScaleX = 1.0f;
             if (autoScaleY >= 1.0f)
                autoScaleY = 1.0f;

             autoScaleX = autoScaleY = (autoScaleX + autoScaleY) / 2;

             /* Because we modified the scale after calculating
              * the output rect, we need to recalculate again
              */

             FWCalculateOutputRect (w);

         }

         float scaleX = autoScaleX - (1 - fww->transform.scaleX);
         float scaleY = autoScaleY - (1 - fww->transform.scaleY);

         /* Actually Transform the window */

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	/* Adjust the window in the matrix to prepare for transformation */

	if (fww->grab != grabRotate && fww->grab != grabScale)
	{

	    FWCalculateInputOrigin (w,
				    WIN_REAL_X (w) + WIN_REAL_W (w) / 2.0f,
				    WIN_REAL_Y (w) + WIN_REAL_H (w) / 2.0f);
	    FWCalculateOutputOrigin (w,
				     WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w) / 2.0f,
				     WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w) / 2.0f);
	}

	float adjustX, adjustY;

	adjustX = 0.0f;
	adjustY = 0.0f;
	FWModifyMatrix (w, &wTransform,
	                    fww->transform.angX,
	                    fww->transform.angY,
		            fww->transform.angZ,
		            fww->iMidX, fww->iMidY , 0.0f,
		            scaleX, scaleY, 1.0f, adjustX, adjustY, TRUE);

	/* Create rects for input after we've dealt
	 * with output
	 */

	FWCalculateInputRect (w);

	/* Determine if the window is inverted */

	Bool xInvert = FALSE;
	Bool yInvert = FALSE;

	Bool needsInvert = FALSE;
	float renX = fabs (fmodf (fww->transform.angX, 360.0f));
	float renY = fabs (fmodf (fww->transform.angY, 360.0f));

	if (90 < renX && renX < 270)
	    xInvert = TRUE;

	if (90 < renY && renY < 270)
	    yInvert = TRUE;

	if ((xInvert || yInvert) && !(xInvert && yInvert))
	    needsInvert = TRUE;

	if (needsInvert)
	    glCullFace (invertCull);

	UNWRAP(fws, w->screen, paintWindow);
	status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
	WRAP(fws, w->screen, paintWindow, FWPaintWindow);

	if (needsInvert)
	    glCullFace (currentCull);

        }
    else
    {
	UNWRAP(fws, w->screen, paintWindow);
	status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
	WRAP(fws, w->screen, paintWindow, FWPaintWindow);
    }


    // Check if there are rotated windows
    if (!((fww->transform.angX >= 0.0f - 0.05 &&
              fww->transform.angX <= 0.0f + 0.05 ) &&
             (fww->transform.angY >= 0.0f - 0.05 &&
              fww->transform.angY <= 0.0f + 0.05 ) &&
             (fww->transform.angZ >= 0.0f - 0.05 &&
              fww->transform.angZ <= 0.0f + 0.05 ) &&
             (fww->transform.scaleX >= 1.0f - 0.00005 &&
              fww->transform.scaleX <= 1.0f + 0.00005 ) &&
             (fww->transform.scaleY >= 1.0f - 0.00005 &&
              fww->transform.scaleY <= 1.0f + 0.00005 )) && !fww->transformed)
		fww->transformed = TRUE;
	else if (fww->transformed)
	    fww->transformed = FALSE;

    /* There is still animation to be done */

    if (!(((fww->transform.angX >= fww->animate.destAngX - 0.05 &&
      fww->transform.angX <= fww->animate.destAngX + 0.05 ) &&
     (fww->transform.angY >= fww->animate.destAngY - 0.05 &&
      fww->transform.angY <= fww->animate.destAngY + 0.05 ) &&
     (fww->transform.angZ >= fww->animate.destAngZ - 0.05 &&
      fww->transform.angZ <= fww->animate.destAngZ + 0.05 ) &&
     (fww->transform.scaleX >= fww->animate.destScaleX - 0.00005 &&
      fww->transform.scaleX <= fww->animate.destScaleX + 0.00005 ) &&
     (fww->transform.scaleY >= fww->animate.destScaleY - 0.00005 &&
      fww->transform.scaleY <= fww->animate.destScaleY + 0.00005 ))))
    {
		FWDamageArea (w);
		fww->isAnimating = TRUE;
    }
    else if (fww->isAnimating) /* We're done animating now, and we were animating */
    {
	if (FWHandleWindowInputInfo (w))
	    FWAdjustIPW (w);
	fww->isAnimating = FALSE;
    }

    return status;
}

/* Paint the window axis help onto the screen */
Bool
FWPaintOutput(CompScreen *s,
	      const ScreenPaintAttrib *sAttrib,
	      const CompTransform *transform,
	      Region region,
	      CompOutput *output,
	      unsigned int mask)
{

    Bool wasCulled, status;
    CompTransform zTransform;
    float x, y;
    int j;

    FREEWINS_SCREEN (s);
    FREEWINS_DISPLAY (s->display);

    if (fws->transformedWindows)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (fws, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (fws, s, paintOutput, FWPaintOutput);

    if (fwd->axisHelp && fwd->hoverWindow)
    {
	x = WIN_REAL_X(fwd->hoverWindow) + WIN_REAL_W(fwd->hoverWindow)/2.0;
	y = WIN_REAL_Y(fwd->hoverWindow) + WIN_REAL_H(fwd->hoverWindow)/2.0;

        FREEWINS_WINDOW (fwd->hoverWindow);

        float zRad = fww->radius * (freewinsGet3dPercent (s) / 100);

	wasCulled = glIsEnabled (GL_CULL_FACE);
	zTransform = *transform;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &zTransform);

	glPushMatrix ();
	glLoadMatrixf (zTransform.m);

	if (wasCulled)
	    glDisable (GL_CULL_FACE);

    if (freewinsGetShowCircle (s) && freewinsGetRotationAxis (fwd->hoverWindow->screen) == RotationAxisAlwaysCentre)
    {

	glColor4usv  (freewinsGetCircleColor (s));
	glEnable (GL_BLEND);

	glBegin (GL_POLYGON);
	for (j=0; j<360; j += 10)
	    glVertex3f ( x + zRad * cos(D2R(j)), y + zRad * sin(D2R(j)), 0.0 );
	glEnd ();

	glDisable (GL_BLEND);
	glColor4usv  (freewinsGetLineColor (s));
	glLineWidth (3.0);

	glBegin (GL_LINE_LOOP);
	for (j=360; j>=0; j -= 10)
	    glVertex3f ( x + zRad * cos(D2R(j)), y + zRad * sin(D2R(j)), 0.0 );
	glEnd ();

	glBegin (GL_LINE_LOOP);
	for (j=360; j>=0; j -= 10)
	    glVertex3f( x + fww->radius * cos(D2R(j)), y + fww->radius * sin(D2R(j)), 0.0 );
	glEnd ();

    }

    /* Draw the 'gizmo' */

    if (freewinsGetShowGizmo (s))
    {

	glPushMatrix ();

	glTranslatef (x, y, 0.0);

	glScalef (zRad, zRad, zRad / (float)s->width);

        glRotatef (fww->transform.angX, 1.0f, 0.0f, 0.0f);
        glRotatef (fww->transform.angY, 0.0f, 1.0f, 0.0f);
        glRotatef (fww->transform.angZ, 0.0f, 0.0f, 1.0f);

	glLineWidth (4.0f);
        int i;
	    for (i=0; i<3; i++)
	    {
		glPushMatrix ();
		    glColor4f (1.0 * (i==0), 1.0 * (i==1), 1.0 * (i==2), 1.0);
		    glRotatef (90.0, 1.0 * (i==0), 1.0 * (i==1), 1.0 * (i==2));

		    glBegin (GL_LINE_LOOP);
		    for (j=360; j>=0; j -= 10)
			glVertex3f ( cos (D2R (j)), sin (D2R (j)), 0.0 );
		    glEnd ();
		glPopMatrix ();
	    }

	glPopMatrix ();
	glColor4usv (defaultColor);

    }

    /* Draw the bounding box */

    if (freewinsGetShowRegion (s))
    {

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);
    glColor4us (0x2fff, 0x2fff, 0x4fff, 0x4fff);
    glRecti (fww->inputRect.x1, fww->inputRect.y1, fww->inputRect.x2, fww->inputRect.y2);
    glColor4us (0x2fff, 0x2fff, 0x4fff, 0x9fff);
    glBegin (GL_LINE_LOOP);
    glVertex2i (fww->inputRect.x1, fww->inputRect.y1);
    glVertex2i (fww->inputRect.x2, fww->inputRect.y1);
    glVertex2i (fww->inputRect.x1, fww->inputRect.y2);
    glVertex2i (fww->inputRect.x2, fww->inputRect.y2);
    glEnd ();
    glColor4usv (defaultColor);
    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    }

    if (freewinsGetShowCross (s))
    {

	glColor4usv  (freewinsGetCrossLineColor (s));
	glBegin(GL_LINES);
	glVertex3f(x, y - (WIN_REAL_H (fwd->hoverWindow) / 2), 0.0f);
	glVertex3f(x, y + (WIN_REAL_H (fwd->hoverWindow) / 2), 0.0f);
	glEnd ();

	glBegin(GL_LINES);
	glVertex3f(x - (WIN_REAL_W (fwd->hoverWindow) / 2), y, 0.0f);
	glVertex3f(x + (WIN_REAL_W (fwd->hoverWindow) / 2), y, 0.0f);
	glEnd ();

	/* Move to our first corner (TopLeft)  */

	if (fww->input)
	{

	glBegin(GL_LINES);
	glVertex3f(fww->output.shapex1, fww->output.shapey1, 0.0f);
	glVertex3f(fww->output.shapex2, fww->output.shapey2, 0.0f);
	glEnd ();

	glBegin(GL_LINES);
	glVertex3f(fww->output.shapex2, fww->output.shapey2, 0.0f);
	glVertex3f(fww->output.shapex4, fww->output.shapey4, 0.0f);
	glEnd ();

	glBegin(GL_LINES);
	glVertex3f(fww->output.shapex4, fww->output.shapey4, 0.0f);
	glVertex3f(fww->output.shapex3, fww->output.shapey3, 0.0f);
	glEnd ();

	glBegin(GL_LINES);
	glVertex3f(fww->output.shapex3, fww->output.shapey3, 0.0f);
	glVertex3f(fww->output.shapex1, fww->output.shapey1, 0.0f);
	glEnd ();

	}

    }

    if (wasCulled)
	glEnable(GL_CULL_FACE);

    glColor4usv(defaultColor);
    glPopMatrix ();
    }

    return status;
}

void
FWDonePaintScreen (CompScreen *s)
{
	FREEWINS_DISPLAY (s->display);
	FREEWINS_SCREEN (s);

	if (fwd->axisHelp && fwd->hoverWindow)
	{
		FREEWINS_WINDOW (fwd->hoverWindow);

		REGION region;

		region.rects = &region.extents;
		region.numRects = region.size = 1;

		region.extents.x1 = MIN (WIN_REAL_X (fwd->hoverWindow),
					WIN_REAL_X (fwd->hoverWindow)
					+ WIN_REAL_W (fwd->hoverWindow)
					 / 2.0f - fww->radius);
		region.extents.x2 = MAX (WIN_REAL_X (fwd->hoverWindow),
					WIN_REAL_X (fwd->hoverWindow)
					+ WIN_REAL_W (fwd->hoverWindow)
					 / 2.0f + fww->radius);

		region.extents.y1 = MIN (WIN_REAL_Y (fwd->hoverWindow),
					WIN_REAL_Y (fwd->hoverWindow)
					+ WIN_REAL_H (fwd->hoverWindow)
					 / 2.0f - fww->radius);
		region.extents.y2 = MAX (WIN_REAL_Y (fwd->hoverWindow),
					WIN_REAL_Y (fwd->hoverWindow)
					+ WIN_REAL_H (fwd->hoverWindow)
					 / 2.0f + fww->radius);

		damageScreenRegion (s, &region);
	}

	UNWRAP (fws, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP (fws, s, donePaintScreen, FWDonePaintScreen);
}

/* Damage the Window Rect */
Bool
FWDamageWindowRect(CompWindow *w,
		   Bool initial,
		   BoxPtr rect)
{

    Bool status = TRUE;
    FREEWINS_SCREEN(w->screen);
    FREEWINS_WINDOW(w);

    if (fww->transformed)
        FWDamageArea (w);
    else
        status = FALSE;

    /**
     * Special situations where we must damage the screen
     * i.e when we are playing with windows and wobbly is
     * enabled
     */

    if ((fww->grab == grabMove && !freewinsGetImmediateMoves (w->screen))
        || (fww->isAnimating || w->grabbed))
        damageScreen (w->screen);

    UNWRAP(fws, w->screen, damageWindowRect);
    status |= (*w->screen->damageWindowRect)(w, initial, rect);
    WRAP(fws, w->screen, damageWindowRect, FWDamageWindowRect);

    return status;
}
