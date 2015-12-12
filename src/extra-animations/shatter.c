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

Bool fxShatterInit(CompWindow * w)
{
    CompScreen *s = w->screen;
    ANIMPLUS_DISPLAY (s->display);
    ANIMPLUS_WINDOW (w);

    if (!ad->animAddonFunc->polygonsAnimInit(w))
        return FALSE;

   // CompScreen *s = w->screen;
    int i,static_polygon;
    int screen_height = s->outputDev[outputDeviceForWindow(w)].height;

    ad->animAddonFunc->tessellateIntoGlass( w,
                animGetI( w, ANIMPLUS_SCREEN_OPTION_SHATTER_NUM_SPOKES),
                animGetI( w, ANIMPLUS_SCREEN_OPTION_SHATTER_NUM_TIERS),
                             1); //can't really see how thick it is...

    PolygonSet *pset = aw->eng->polygonSet;
    PolygonObject *p = pset->polygons;

    for (i = 0; i < pset->nPolygons; i++, p++)
    {
        p->rotAxis.x = 0;
        p->rotAxis.y = 0;
        p->rotAxis.z = 1;

        static_polygon = 1;

        p->finalRelPos.x = 0;
        p->finalRelPos.y = static_polygon *
                            (-p->centerPosStart.y + screen_height);
        p->finalRelPos.z = 0;
        if (p->finalRelPos.y)
            p->finalRotAng = RAND_FLOAT() * 120 * ( RAND_FLOAT() < 0.5 ? -1 : 1 );
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
