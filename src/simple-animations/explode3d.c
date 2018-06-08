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

/*Bool
fxExplodeInit (CompWindow * w)
{
    CompScreen *s = w->screen;
    ANIMEG_DISPLAY (s->display);
    ANIMEG_WINDOW (w);

    if (!ad->animAddonFunc->polygonsAnimInit (w))
	return FALSE;

    switch (animGetI (w, ANIMEG_SCREEN_OPTION_EXPLODE_TESS))
    {
    case PolygonTessRect:
	if (!ad->animAddonFunc->tessellateIntoRectangles
	    (w,
	     animGetI (w, ANIMEG_SCREEN_OPTION_EXPLODE_GRIDSIZE_X),
	     animGetI (w, ANIMEG_SCREEN_OPTION_EXPLODE_GRIDSIZE_Y),
	     animGetF (w, ANIMEG_SCREEN_OPTION_EXPLODE_THICKNESS)))
	    return FALSE;
	break;
    case PolygonTessHex:
	if (!ad->animAddonFunc->tessellateIntoHexagons
	    (w,
	     animGetI (w, ANIMEG_SCREEN_OPTION_EXPLODE_GRIDSIZE_X),
	     animGetI (w, ANIMEG_SCREEN_OPTION_EXPLODE_GRIDSIZE_Y),
	     animGetF (w, ANIMEG_SCREEN_OPTION_EXPLODE_THICKNESS)))
	    return FALSE;
	break;
    default:
	return FALSE;
    }

    PolygonSet *pset = aw->eng->polygonSet;
    PolygonObject *p = pset->polygons;
    double sqrt2 = sqrt (2);

    int i;

    for (i = 0; i < pset->nPolygons; i++, p++)
    {
	p->rotAxis.x = RAND_FLOAT ();
	p->rotAxis.y = RAND_FLOAT ();
	p->rotAxis.z = RAND_FLOAT ();

	float screenSizeFactor = (0.8 * DEFAULT_Z_CAMERA * s->width);
	float speed = screenSizeFactor / 10 * (0.2 + RAND_FLOAT ());

	float xx = 2 * (p->centerRelPos.x - 0.5);
	float yy = 2 * (p->centerRelPos.y - 0.5);

	float x = speed * 2 * (xx + 0.5 * (RAND_FLOAT () - 0.5));
	float y = speed * 2 * (yy + 0.5 * (RAND_FLOAT () - 0.5));

	float distToCenter = sqrt (xx * xx + yy * yy) / sqrt2;
	float moveMult = 1 - distToCenter;
	moveMult = moveMult < 0 ? 0 : moveMult;
	float zbias = 0.1;
	float z = speed * 10 *
	    (zbias + RAND_FLOAT () *
	     pow (moveMult, 0.5));

	p->finalRelPos.x = x;
	p->finalRelPos.y = y;
	p->finalRelPos.z = z;
	p->finalRotAng = RAND_FLOAT () * 540 - 270;
    }
    pset->allFadeDuration = 0.3f;
    pset->doDepthTest = TRUE;
    pset->doLighting = TRUE;
    pset->correctPerspective = CorrectPerspectivePolygon;
    pset->backAndSidesFadeDur = 0.2f;

    aw->com->animTotalTime /= EXPLODE_PERCEIVED_T;
    aw->com->animRemainingTime = aw->com->animTotalTime;

    return TRUE;
}*/
