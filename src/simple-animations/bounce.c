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

// =====================  Effect: Bounce  =========================

float
fxBounceAnimProgress (CompWindow *w)
{
    ANIMSIM_DISPLAY (w->screen->display);
    ANIMSIM_WINDOW  (w);
    float forwardProgress = (*ad->animBaseFunc->defaultAnimProgress) (w);
    float forwardProgressInc = 1.0f / aw->bounceCount;

    /* last bounce, enure we are going for 0.0 */
    aw->currBounceProgress = (((1 - forwardProgress) - aw->lastProgressMax) / forwardProgressInc);

    if (aw->currBounceProgress > 1.0f)
    {
	aw->currentScale = aw->targetScale;
	aw->targetScale = -aw->targetScale + aw->targetScale / 2.0f;
	aw->lastProgressMax = 1.0f - forwardProgress;
	aw->currBounceProgress = 0.0f;
	aw->nBounce++;
    }

    return forwardProgress;
}

static void
applyBounceTransform (CompWindow *w)
{
    ANIMSIM_WINDOW (w);
    float scale = 1.0f - (aw->targetScale * (aw->currBounceProgress) + aw->currentScale * (1.0f - aw->currBounceProgress));

    CompTransform *transform = &aw->com->transform;

    matrixTranslate (transform, WIN_X (w) + WIN_W (w) / 2.0f,
				WIN_Y (w) + WIN_H (w) / 2.0f, 0.0f);

    matrixScale (transform, scale, scale, 1.0f);

    matrixTranslate (transform, -(WIN_X (w) + WIN_W (w) / 2.0f),
				-(WIN_Y (w) + WIN_H (w) / 2.0f), 0.0f);

}

void
fxBounceAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyBounceTransform (w);
}

void
fxBounceUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
    ANIMSIM_WINDOW(w);

    float forwardProgress = fxBounceAnimProgress (w);

    if (animGetB (w, ANIMSIM_SCREEN_OPTION_BOUNCE_FADE))
	wAttrib->opacity = aw->com->storedOpacity * (1 - forwardProgress);
}

void
fxBounceUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxBounceInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);
    ANIMSIM_WINDOW (w);

    aw->bounceCount = animGetI (w, ANIMSIM_SCREEN_OPTION_BOUNCE_NUMBER);
    aw->nBounce     = 1;
    aw->targetScale = animGetF (w, ANIMSIM_SCREEN_OPTION_BOUNCE_MIN_SIZE);
    aw->currentScale = animGetF (w, ANIMSIM_SCREEN_OPTION_BOUNCE_MAX_SIZE);
    aw->bounceNeg   = FALSE;
    aw->currBounceProgress = 0.0f;
    aw->lastProgressMax = 0.0f;

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}
