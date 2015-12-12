/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
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

Bool fxHelixInit(CompWindow * w)
{
    CompScreen *s = w->screen;
    ANIMPLUS_DISPLAY (s->display);
    ANIMPLUS_WINDOW (w);

    if (!ad->animAddonFunc->polygonsAnimInit(w))
        return FALSE;

   // CompScreen *s = w->screen;

    int gridsizeY = animGetI( w, ANIMPLUS_SCREEN_OPTION_HELIX_GRIDSIZE_Y);

    ad->animAddonFunc->tessellateIntoRectangles(w,
                             1,
                             gridsizeY,
                             animGetF( w, ANIMPLUS_SCREEN_OPTION_HELIX_THICKNESS));

    PolygonSet *pset = aw->eng->polygonSet;
    PolygonObject *p = pset->polygons;


    int i;
    for (i = 0; i < pset->nPolygons; i++, p++)
    {

    //rotate around y axis normally, or the z axis if the effect is in vertical mode
    p->rotAxis.x = 0;

    if (animGetB( w, ANIMPLUS_SCREEN_OPTION_HELIX_DIRECTION))
    {
        p->rotAxis.y = 0;
        p->rotAxis.z = 1;
    } else {
        p->rotAxis.y = 1;
        p->rotAxis.z = 0;
    }

    //only move the pieces in a 'vertical' rotation
    p->finalRelPos.x = 0;

    if (animGetB( w, ANIMPLUS_SCREEN_OPTION_HELIX_DIRECTION))
        p->finalRelPos.y = -1 * ((w->height)/ gridsizeY) * (i -  gridsizeY/2);
    else
        p->finalRelPos.y = 0;


    p->finalRelPos.z = 0;

    //determine how long, and what direction to spin
    int numberOfTwists = animGetI( w, ANIMPLUS_SCREEN_OPTION_HELIX_NUM_TWISTS);
    int spin_dir = animGetI( w, ANIMPLUS_SCREEN_OPTION_HELIX_SPIN_DIRECTION);

    if (spin_dir)
        p->finalRotAng = 270 - ( 2 * numberOfTwists * i);
    else
        p->finalRotAng = ( 2 * numberOfTwists * i) - 270;


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
