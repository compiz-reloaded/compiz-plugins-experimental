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

// =====================  Effect: Expand  =========================

float
fxExpandAnimProgress (CompWindow *w)
{
    ANIMSIM_DISPLAY (w->screen->display);
    float forwardProgress = (*ad->animBaseFunc->defaultAnimProgress) (w);

    return (*ad->animBaseFunc->decelerateProgress) (forwardProgress);
}

static void
applyExpandTransform (CompWindow *w)
{
    ANIMSIM_WINDOW (w);

    CompTransform *transform = &aw->com->transform;
    float defaultXScale = 0.3f;
    float forwardProgress;
    float expandProgress;
    float expandPhaseEnd = 0.5f;

    forwardProgress = fxExpandAnimProgress (w);

    if ((1 - forwardProgress) < expandPhaseEnd)
	expandProgress = (1 - forwardProgress) / expandPhaseEnd;
    else
	expandProgress = 1.0f;

    // animation movement
    matrixTranslate (transform, WIN_X (w) + WIN_W (w) / 2.0f,
				WIN_Y (w) + WIN_H (w) / 2.0f,
				0.0f);

    matrixScale (transform, defaultXScale + (1.0f - defaultXScale) * expandProgress,
			    (1 - forwardProgress), 0.0f);

    matrixTranslate (transform, -(WIN_X (w) + WIN_W (w) / 2.0f),
				-(WIN_Y (w) + WIN_H (w) / 2.0f),
				0.0f);

}

void
fxExpandAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyExpandTransform (w);
}

void
fxExpandUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
}

void
fxExpandUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxExpandInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}
