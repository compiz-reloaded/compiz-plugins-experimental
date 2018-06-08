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

float
fxFlyinAnimProgress (CompWindow *w)
{
    ANIMSIM_DISPLAY (w->screen->display);
    float forwardProgress = (*ad->animBaseFunc->defaultAnimProgress) (w);

    return (*ad->animBaseFunc->decelerateProgress) (forwardProgress);
}

static void
applyFlyinTransform (CompWindow *w)
{
    ANIMSIM_WINDOW (w);

    CompTransform *transform = &aw->com->transform;
    float offsetX, offsetY;
    float xTrans, yTrans;
    float forwardProgress;

    int direction = animGetI (w, ANIMSIM_SCREEN_OPTION_FLYIN_DIRECTION);

    switch (direction)
    {
	case 0:
	    offsetX = 0;
	    offsetY = animGetF (w, ANIMSIM_SCREEN_OPTION_FLYIN_DISTANCE);
	    break;
	case 1:
	    offsetX = animGetF (w, ANIMSIM_SCREEN_OPTION_FLYIN_DISTANCE);
	    offsetY = 0;
	    break;
	case 2:
	    offsetX = 0;
	    offsetY = -animGetF (w, ANIMSIM_SCREEN_OPTION_FLYIN_DISTANCE);
	    break;
	case 3:
	    offsetX = -animGetF (w, ANIMSIM_SCREEN_OPTION_FLYIN_DISTANCE);
	    offsetY = 0;
	    break;
	case 4:
	    offsetX = animGetF (w, ANIMSIM_SCREEN_OPTION_FLYIN_DIRECTION_X);
	    offsetY = animGetF (w, ANIMSIM_SCREEN_OPTION_FLYIN_DIRECTION_Y);
	    break;
	default:
	    return;
    }

    forwardProgress = fxFlyinAnimProgress (w);
    xTrans = -(forwardProgress * offsetX);
    yTrans = -(forwardProgress * offsetY);
    Point3d translation = {xTrans, yTrans, 0};

    // animation movement
    matrixTranslate (transform, translation.x, translation.y, translation.z);

}

void
fxFlyinAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyFlyinTransform (w);
}

void
fxFlyinUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
    ANIMSIM_WINDOW(w);

    float forwardProgress = fxFlyinAnimProgress (w);

    if (animGetB (w, ANIMSIM_SCREEN_OPTION_FLYIN_FADE))
	wAttrib->opacity = aw->com->storedOpacity * (1 - forwardProgress);
}

void
fxFlyinUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxFlyinInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}
