/**
 * Compiz Fusion Freewins plugin
 *
 * util.c
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
#include <stdlib.h>


/* ------ Utility Functions ---------------------------------------------*/

/* Rotate and project individual vectors */
void
FWRotateProjectVector (CompWindow *w,
		       CompVector vector,
                       CompTransform transform,
		       GLdouble *resultX,
		       GLdouble *resultY,
		       GLdouble *resultZ,
		       Bool report)
{

    matrixMultiplyVector(&vector, &vector, &transform);

    GLint viewport[4]; // Viewport
    GLdouble modelview[16]; // Modelview Matrix
    GLdouble projection[16]; // Projection Matrix

    glGetIntegerv (GL_VIEWPORT, viewport);
    glGetDoublev (GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev (GL_PROJECTION_MATRIX, projection);

    gluProject (vector.x, vector.y, vector.z,
                modelview, projection, viewport,
                resultX, resultY, resultZ);

    /* Y must be negated */
    *resultY = w->screen->height - *resultY;
}

// Scales z by 0 and does perspective distortion so that it
// looks the same wherever on the screen

/* This code taken from animation.c,
 * Copyright (c) 2006 Erkin Bahceci
 */
static void
perspectiveDistortAndResetZ (CompScreen *s,
			     CompTransform *transform)
{
    float v = -1.0 / s->width;
    /*
      This does
      transform = M * transform, where M is
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 0, v,
      0, 0, 0, 1
    */
    float *m = transform->m;
    m[8] = v * m[12];
    m[9] = v * m[13];
    m[10] = v * m[14];
    m[11] = v * m[15];
}

void
FWModifyMatrix  (CompWindow *w, CompTransform *mTransform,
                 float angX, float angY, float angZ,
                 float tX, float tY, float tZ,
                 float scX, float scY, float scZ,
                 float adjustX, float adjustY, Bool paint)
{
    /* Create our transformation Matrix */

    matrixTranslate(mTransform,
	    tX,
	    tY, 0.0);
    if (paint)
	perspectiveDistortAndResetZ (w->screen, mTransform);
    else
	matrixScale (mTransform, 1.0f, 1.0f, 1.0f / w->screen->width);
    matrixRotate (mTransform, angX, 1.0f, 0.0f, 0.0f);
    matrixRotate (mTransform, angY, 0.0f, 1.0f, 0.0f);
    matrixRotate (mTransform, angZ, 0.0f, 0.0f, 1.0f);
    matrixScale(mTransform, scX, 1.0f, 0.0f);
    matrixScale(mTransform, 1.0f, scY, 0.0f);
    matrixTranslate(mTransform,
            -(tX),
            -(tY), 0.0f);
}

/*
static float det3(float m00, float m01, float m02,
		 float m10, float m11, float m12,
		 float m20, float m21, float m22){

    float ret = 0.0;

    ret += m00 * m11 * m22 - m21 * m12 * m00;
    ret += m01 * m12 * m20 - m22 * m10 * m01;
    ret += m02 * m10 * m21 - m20 * m11 * m02;

    return ret;
}

static void FWFindInverseMatrix(CompTransform *m, CompTransform *r){
    float *mm = m->m;
    float d, c[16];

    d = mm[0] * det3(mm[5], mm[6], mm[7],
			mm[9], mm[10], mm[11],
			 mm[13], mm[14], mm[15]) -

	  mm[1] * det3(mm[4], mm[6], mm[7],
			mm[8], mm[10], mm[11],
			 mm[12], mm[14], mm[15]) +

	  mm[2] * det3(mm[4], mm[5], mm[7],
			mm[8], mm[9], mm[11],
			 mm[12], mm[13], mm[15]) -

	  mm[3] * det3(mm[4], mm[5], mm[6],
			mm[8], mm[9], mm[10],
			 mm[12], mm[13], mm[14]);

    c[0] = det3(mm[5], mm[6], mm[7],
		  mm[9], mm[10], mm[11],
		  mm[13], mm[14], mm[15]);
    c[1] = -det3(mm[4], mm[6], mm[7],
		  mm[8], mm[10], mm[11],
		  mm[12], mm[14], mm[15]);
    c[2] = det3(mm[4], mm[5], mm[7],
		  mm[8], mm[9], mm[11],
		  mm[12], mm[13], mm[15]);
    c[3] = -det3(mm[4], mm[5], mm[6],
		  mm[8], mm[9], mm[10],
		  mm[12], mm[13], mm[14]);

    c[4] = -det3(mm[1], mm[2], mm[3],
		  mm[9], mm[10], mm[11],
		  mm[13], mm[14], mm[15]);
    c[5] = det3(mm[0], mm[2], mm[3],
		  mm[8], mm[10], mm[11],
		  mm[12], mm[14], mm[15]);
    c[6] = -det3(mm[0], mm[1], mm[3],
		  mm[8], mm[9], mm[11],
		  mm[12], mm[13], mm[15]);
    c[7] = det3(mm[0], mm[1], mm[2],
		  mm[8], mm[9], mm[10],
		  mm[12], mm[13], mm[14]);

    c[8] = det3(mm[1], mm[2], mm[3],
		  mm[5], mm[6], mm[7],
		  mm[13], mm[14], mm[15]);
    c[9] = -det3(mm[0], mm[2], mm[3],
		  mm[4], mm[6], mm[7],
		  mm[12], mm[14], mm[15]);
    c[10] = det3(mm[0], mm[1], mm[3],
		  mm[4], mm[5], mm[7],
		  mm[12], mm[13], mm[15]);
    c[11] = -det3(mm[0], mm[1], mm[2],
		  mm[4], mm[5], mm[6],
		  mm[12], mm[13], mm[14]);

    c[12] = -det3(mm[1], mm[2], mm[3],
		  mm[5], mm[6], mm[7],
		  mm[9], mm[10], mm[11]);
    c[13] = det3(mm[0], mm[2], mm[3],
		  mm[4], mm[6], mm[7],
		  mm[8], mm[10], mm[11]);
    c[14] = -det3(mm[0], mm[1], mm[3],
		  mm[4], mm[5], mm[7],
		  mm[8], mm[9], mm[11]);
    c[15] = det3(mm[0], mm[1], mm[2],
		  mm[4], mm[5], mm[6],
		  mm[8], mm[9], mm[10]);


    r->m[0] = c[0] / d;
    r->m[1] = c[4] / d;
    r->m[2] = c[8] / d;
    r->m[3] = c[12] / d;

    r->m[4] = c[1] / d;
    r->m[5] = c[5] / d;
    r->m[6] = c[9] / d;
    r->m[7] = c[13] / d;

    r->m[8] = c[2] / d;
    r->m[9] = c[6] / d;
    r->m[10] = c[10] / d;
    r->m[11] = c[14] / d;

    r->m[12] = c[3] / d;
    r->m[13] = c[7] / d;
    r->m[14] = c[11] / d;
    r->m[15] = c[15] / d;

    return;
}
*/

/* Create a rect from 4 screen points */
Box
FWCreateSizedRect (float xScreen1,
                   float xScreen2,
                   float xScreen3,
                   float xScreen4,
                   float yScreen1,
                   float yScreen2,
                   float yScreen3,
                   float yScreen4)
{
        float leftmost, rightmost, topmost, bottommost;
        Box rect;

        /* Left most point */

        leftmost = xScreen1;

        if (xScreen2 <= leftmost)
            leftmost = xScreen2;

        if (xScreen3 <= leftmost)
            leftmost = xScreen3;

        if (xScreen4 <= leftmost)
            leftmost = xScreen4;

        /* Right most point */

        rightmost = xScreen1;

        if (xScreen2 >= rightmost)
            rightmost = xScreen2;

        if (xScreen3 >= rightmost)
            rightmost = xScreen3;

        if (xScreen4 >= rightmost)
            rightmost = xScreen4;

        /* Top most point */

        topmost = yScreen1;

        if (yScreen2 <= topmost)
            topmost = yScreen2;

        if (yScreen3 <= topmost)
            topmost = yScreen3;

        if (yScreen4 <= topmost)
            topmost = yScreen4;

        /* Bottom most point */

        bottommost = yScreen1;

        if (yScreen2 >= bottommost)
            bottommost = yScreen2;

        if (yScreen3 >= bottommost)
            bottommost = yScreen3;

        if (yScreen4 >= bottommost)
            bottommost = yScreen4;

        rect.x1 = leftmost;
        rect.x2 = rightmost;
        rect.y1 = topmost;
        rect.y2 = bottommost;

        return rect;
}

Box
FWCalculateWindowRect (CompWindow *w,
                       CompVector c1,
                       CompVector c2,
                       CompVector c3,
                       CompVector c4)
{

        FREEWINS_WINDOW (w);

        CompTransform transform;
        GLdouble xScreen1 = 0.0f, yScreen1 = 0.0f, zScreen1 = 0.0f;
        GLdouble xScreen2 = 0.0f, yScreen2 = 0.0f, zScreen2 = 0.0f;
        GLdouble xScreen3 = 0.0f, yScreen3 = 0.0f, zScreen3 = 0.0f;
        GLdouble xScreen4 = 0.0f, yScreen4 = 0.0f, zScreen4 = 0.0f;

        matrixGetIdentity(&transform);
        FWModifyMatrix (w, &transform,
                        fww->transform.angX,
                        fww->transform.angY,
                        fww->transform.angZ,
                        fww->iMidX, fww->iMidY, 0.0f,
                        fww->transform.scaleX,
                        fww->transform.scaleY, 0.0f, 0.0f, 0.0f, FALSE);

        FWRotateProjectVector(w, c1, transform, &xScreen1, &yScreen1, &zScreen1, FALSE);
        FWRotateProjectVector(w, c2, transform, &xScreen2, &yScreen2, &zScreen2, FALSE);
        FWRotateProjectVector(w, c3, transform, &xScreen3, &yScreen3, &zScreen3, FALSE);
        FWRotateProjectVector(w, c4, transform, &xScreen4, &yScreen4, &zScreen4, FALSE);

	/* Save the non-rectangular points so that we can shape the rectangular IPW */

	fww->output.shapex1 = xScreen1;
	fww->output.shapex2 = xScreen2;
	fww->output.shapex3 = xScreen3;
	fww->output.shapex4 = xScreen4;
	fww->output.shapey1 = yScreen1;
	fww->output.shapey2 = yScreen2;
	fww->output.shapey3 = yScreen3;
	fww->output.shapey4 = yScreen4;


        return FWCreateSizedRect(xScreen1, xScreen2, xScreen3, xScreen4,
                                 yScreen1, yScreen2, yScreen3, yScreen4);

}

void
FWCalculateOutputRect (CompWindow *w)
{
    if (w)
    {

    FREEWINS_WINDOW (w);

    CompVector corner1 =
    { .v = { WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w), 1.0f, 1.0f } };
    CompVector corner2 =
    { .v = { WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w), 1.0f, 1.0f } };
    CompVector corner3 =
    { .v = { WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w), 1.0f, 1.0f } };
    CompVector corner4 =
    { .v = { WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w), 1.0f, 1.0f } };

    fww->outputRect = FWCalculateWindowRect (w, corner1, corner2, corner3, corner4);
    }
}

void
FWCalculateInputRect (CompWindow *w)
{

    if (w)
    {

    FREEWINS_WINDOW (w);

    CompVector corner1 =
    { .v = { WIN_REAL_X (w), WIN_REAL_Y (w), 1.0f, 1.0f } };
    CompVector corner2 =
    { .v = { WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w), 1.0f, 1.0f } };
    CompVector corner3 =
    { .v = { WIN_REAL_X (w), WIN_REAL_Y (w) + WIN_REAL_H (w), 1.0f, 1.0f } };
    CompVector corner4 =
    { .v = { WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w) + WIN_REAL_H (w), 1.0f, 1.0f } };

    fww->inputRect = FWCalculateWindowRect (w, corner1, corner2, corner3, corner4);
    }

}

void
FWCalculateInputOrigin (CompWindow *w, float x, float y)
{

    FREEWINS_WINDOW (w);

    fww->iMidX = x;
    fww->iMidY = y;
}

void
FWCalculateOutputOrigin (CompWindow *w, float x, float y)
{

    FREEWINS_WINDOW (w);

    float dx, dy;

    dx = x - WIN_OUTPUT_X (w);
    dy = y - WIN_OUTPUT_Y (w);

    fww->oMidX = WIN_OUTPUT_X (w) + dx * fww->transform.scaleX;
    fww->oMidY = WIN_OUTPUT_Y (w) + dy * fww->transform.scaleY;
}

/* Change angles more than 360 into angles out of 360 */
/*static int FWMakeIntoOutOfThreeSixty (int value)
{
    while (value > 0)
    {
        value -= 360;
    }

    if (value < 0)
        value += 360;

    return value;
}*/

/* Determine if we clicked in the z-axis region */
void
FWDetermineZAxisClick (CompWindow *w,
					  int px,
					  int py,
					  Bool motion)
{
    FREEWINS_WINDOW (w);

    Bool directionChange = FALSE;

    if (!fww->can2D && motion)
    {

        static int steps;

        /* Check if we are going in a particular 3D direction
         * because if we are going left/right and we suddenly
         * change to 2D mode this would not be expected behavior.
         * It is only if we have a change in direction that we want
         * to change to 2D rotation.
         */

        Direction direction;

        static int ddx, ddy;

        unsigned int dx = pointerX - lastPointerX;
        unsigned int dy = pointerY - lastPointerY;

        ddx += dx;
        ddy += dy;

        if (steps >= 10)
        {
            if (ddx > ddy)
                direction = LeftRight;
            else
                direction = UpDown;

            if (fww->direction != direction)
                directionChange = TRUE;

            fww->direction = direction;
        }

        steps++;

    }
    else
        directionChange = TRUE;

    if (directionChange)
    {

        float clickRadiusFromCenter;

        int x = (WIN_REAL_X(w) + WIN_REAL_W(w)/2.0);
	    int y = (WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0);

        clickRadiusFromCenter = sqrt(pow((x - px), 2) + pow((y - py), 2));

        if (clickRadiusFromCenter > fww->radius * (freewinsGet3dPercent (w->screen) / 100))
        {
            fww->can2D = TRUE;
            fww->can3D = FALSE;
        }
        else
        {
            fww->can2D = FALSE;
            fww->can3D = TRUE;
        }
    }
}

/* Check to see if we can shape a window */
Bool
FWCanShape (CompWindow *w)
{

    if (!freewinsGetShapeInput (w->screen))
	return FALSE;

    if (!w->screen->display->shapeExtension)
	return FALSE;

    if (!matchEval (freewinsGetShapeWindowTypes (w->screen), w))
	return FALSE;

    return TRUE;
}

/* Checks if w is a ipw and returns the real window */
CompWindow *
FWGetRealWindow (CompWindow *w)
{
    FWWindowInputInfo *info;

    FREEWINS_SCREEN (w->screen);

    for (info = fws->transformedWindows; info; info = info->next)
    {
	if (w->id == info->ipw)
	    return info->w;
    }

    return NULL;
}

void
FWHandleSnap (CompWindow *w)
{
    FREEWINS_WINDOW (w);
    FREEWINS_DISPLAY (w->screen->display);

    /* Handle Snapping */
    if (freewinsGetSnap (w->screen) || fwd->snap)
    {
        int snapFactor = freewinsGetSnapThreshold (w->screen);
        fww->animate.destAngX = ((int) (fww->transform.unsnapAngX) / snapFactor) * snapFactor;
        fww->animate.destAngY = ((int) (fww->transform.unsnapAngY) / snapFactor) * snapFactor;
        fww->animate.destAngZ = ((int) (fww->transform.unsnapAngZ) / snapFactor) * snapFactor;
        fww->transform.scaleX =
        ((float) ( (int) (fww->transform.unsnapScaleX * (21 - snapFactor) + 0.5))) / (21 - snapFactor);
        fww->transform.scaleY =
        ((float) ( (int) (fww->transform.unsnapScaleY * (21 - snapFactor) + 0.5))) / (21 - snapFactor);
    }
}
