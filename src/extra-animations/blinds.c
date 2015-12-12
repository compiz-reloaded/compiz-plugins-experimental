/*
 * Animation plugin for compiz/beryl
 *
 * blinds.c
 *
 * Copyright : (C) 2008 Kevin DuBois
 * E-mail    : kdub432@gmail.com
 *
 * Based on other animations by
 *           : Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Which were based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
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

#include "animationplus.h"

Bool fxBlindsInit( CompWindow * w)
{
    CompScreen *s = w->screen;
    ANIMPLUS_DISPLAY (s->display);
    ANIMPLUS_WINDOW (w);

    if (!ad->animAddonFunc->polygonsAnimInit (w))
        return FALSE;


    ad->animAddonFunc->tessellateIntoRectangles(w,
                      animGetI( w, ANIMPLUS_SCREEN_OPTION_BLINDS_GRIDX),
                      1,
                      animGetF( w, ANIMPLUS_SCREEN_OPTION_BLINDS_THICKNESS));

    PolygonSet *pset = aw->eng->polygonSet;
    PolygonObject *p = pset->polygons;

    int i;

    for (i = 0; i < pset->nPolygons; i++, p++)
    {
    //rotate around y axis
    p->rotAxis.x = 0;
    p->rotAxis.y = 1;
    p->rotAxis.z = 0;

    //dont translate the pieces
    p->finalRelPos.x = 0;
    p->finalRelPos.y = 0;
    p->finalRelPos.z = 0;

    int numberOfHalfTwists = animGetI( w, ANIMPLUS_SCREEN_OPTION_BLINDS_HALFTWISTS);
    p->finalRotAng = 180 * numberOfHalfTwists ;
    }


    pset->allFadeDuration = 0.4f;
    pset->backAndSidesFadeDur = 0.2f;
    pset->doDepthTest = TRUE;
    pset->doLighting = TRUE;
    pset->correctPerspective = CorrectPerspectivePolygon;

    aw->com->animTotalTime /= EXPLODE_PERCEIVED_T;
    aw->com->animRemainingTime = aw->com->animTotalTime;

    return TRUE;
}
