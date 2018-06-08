/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@compiz.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "animationsim.h"

// =====================  Effect: Flyin  =========================

// Scales z by 0 and does perspective distortion so that it
// looks the same wherever on the screen
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

float
fxRotateinAnimProgress (CompWindow *w)
{
    ANIMSIM_DISPLAY (w->screen->display);
    float forwardProgress = (*ad->animBaseFunc->defaultAnimProgress) (w);

    return (*ad->animBaseFunc->decelerateProgress) (forwardProgress);
}

static void
applyRotateinTransform (CompWindow *w)
{
    ANIMSIM_WINDOW (w);

    CompTransform *transform = &aw->com->transform;
    float xRot, yRot;
    float angleX, angleY;
    float originX, originY;
    float forwardProgress;

    int direction = animGetI (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_DIRECTION);

    switch (direction)
    {
	case 1:
	    angleX = 0;
	    angleY = -animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    originX = WIN_X (w);
	    originY = WIN_Y (w) + WIN_H (w);
	    break;
	case 2:
	    angleX = animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    angleY = 0;
	    originX = WIN_X (w);
	    originY = WIN_Y (w);
	    break;
	case 3:
	    angleX = 0;
	    angleY = animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    originX = WIN_X (w);
	    originY = WIN_Y (w);
	    break;
	case 4:
	    angleX = -animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    angleY = 0;
	    originX = WIN_X (w) + WIN_W (w);
	    originY = WIN_Y (w);
	    break;
	default:
	    return;
    }

    forwardProgress = fxRotateinAnimProgress (w);
    xRot = (forwardProgress * angleX);
    yRot = (forwardProgress * angleY);

    matrixTranslate (transform, WIN_X (w) + WIN_W (w) / 2.0f,
				WIN_Y (w) + WIN_H (w) / 2.0f,
				0.0f);

    perspectiveDistortAndResetZ (w->screen, transform);

    matrixTranslate (transform, -(WIN_X (w) + WIN_W (w) / 2.0f),
				-(WIN_Y (w) + WIN_H (w) / 2.0f),
				0.0f);

    // animation movement
    matrixTranslate (transform, originX, originY, 0.0f);

    matrixRotate (transform, yRot, 1.0f, 0.0f, 0.0f);
    matrixRotate (transform, xRot, 0.0f, 1.0f, 0.0f);

    matrixTranslate (transform, -originX, -originY, 0.0f);

}

void
fxRotateinAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyRotateinTransform (w);
}

void
fxRotateinUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
}

void
fxRotateinUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxRotateinInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}

void fxRotateinPrePaintWindow (CompWindow *w)
{
    float forwardProgress = fxRotateinAnimProgress (w);
    float xRot, yRot;
    float angleX, angleY;
    Bool  xInvert = FALSE, yInvert = FALSE;
    int currentCull, invertCull;

    glGetIntegerv (GL_CULL_FACE_MODE, &currentCull);
    invertCull = (currentCull == GL_BACK) ? GL_FRONT : GL_BACK;

    int direction = animGetI (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_DIRECTION);

    switch (direction)
    {
	case 1:
	    angleX = 0;
	    angleY = -animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    break;
	case 2:
	    angleX = animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    angleY = 0;
	    break;
	case 3:
	    angleX = 0;
	    angleY = animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    break;
	case 4:
	    angleX = -animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    angleY = 0;
	    break;
	default:
	    return;
    }

    /* FIXME: This could be fancy vectorial normal direction calculation */

    xRot = fabs(fmodf(forwardProgress * angleX, 360.0f));
    yRot = fabs(fmodf(forwardProgress * angleY, 360.0f));

    if (xRot > 90.0f && xRot > 270.0f)
	xInvert = TRUE;

    if (yRot > 90.0f && yRot > 270.0f)
	yInvert = TRUE;

    if ((xInvert || yInvert) && !(xInvert && yInvert))
	glCullFace (invertCull);
}

void fxRotateinPostPaintWindow (CompWindow * w)
{

    float forwardProgress = fxRotateinAnimProgress (w);
    float xRot, yRot;
    float angleX, angleY;
    Bool  xInvert = FALSE, yInvert = FALSE;
    int currentCull, invertCull;

    glGetIntegerv (GL_CULL_FACE_MODE, &currentCull);
    invertCull = (currentCull == GL_BACK) ? GL_FRONT : GL_BACK;

    int direction = animGetI (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_DIRECTION);

    switch (direction)
    {
	case 1:
	    angleX = 0;
	    angleY = -animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    break;
	case 2:
	    angleX = animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    angleY = 0;
	    break;
	case 3:
	    angleX = 0;
	    angleY = animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    break;
	case 4:
	    angleX = -animGetF (w, ANIMSIM_SCREEN_OPTION_ROTATEIN_ANGLE);
	    angleY = 0;
	    break;
	default:
	    return;
    }

    /* FIXME: This could be fancy vectorial normal direction calculation */

    xRot = fabs(fmodf(forwardProgress * angleX, 360.0f));
    yRot = fabs(fmodf(forwardProgress * angleY, 360.0f));

    if (xRot > 90.0f && xRot > 270.0f)
	xInvert = TRUE;

    if (yRot > 90.0f && yRot > 270.0f)
	yInvert = TRUE;

    /* We have to assume that invertCull will be
     * the actual inversion of our previous cull
     */

    if ((xInvert || yInvert) && !(xInvert && yInvert))
	glCullFace (invertCull);
}

Bool
fxRotateinZoomToIcon (CompWindow *w)
{
    return FALSE;
}
