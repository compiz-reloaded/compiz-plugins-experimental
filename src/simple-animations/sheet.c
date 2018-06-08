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

void
fxSheetsInitGrid (CompWindow *w,
		     int *gridWidth, int *gridHeight)
{
    *gridWidth = 30;
    *gridHeight = 30;
}

Bool
fxSheetsInit (CompWindow * w)
{
    ANIMSIM_WINDOW(w);

    XRectangle *icon = &aw->com->icon;
    int maxWaves;
    float waveAmpMin, waveAmpMax;
    float distance;
    CompWindow *parent;

    for (parent = w->screen->windows; parent; parent = parent->next)
    {
	if (parent->transientFor == w->id && parent->id != w->id)
	    break;
    }

    if (parent)
    {
	icon->x = WIN_X (parent) + WIN_W (parent) / 2.0f;
	icon->y = WIN_Y (parent);
	icon->width = WIN_W (w);
    }
    else
    {
	icon->x = w->screen->width / 2.0f;
	icon->y = 0.0f;
	icon->width = WIN_W (w);
    }

    if (aw->com->curAnimEffect == AnimEffectSheets)
    {
	maxWaves = 0;
	waveAmpMin = 0.0f;
	waveAmpMax = 0.0f;
    }

    if (maxWaves == 0)
    {
	aw->sheetsWaveCount = 0;
	return TRUE;
    }

    // Initialize waves

    distance = WIN_Y(w) + WIN_H(w) - icon->y;

    aw->sheetsWaveCount =
	1 + (float)maxWaves *distance;

    if (!(aw->sheetsWaves))
    {
	aw->sheetsWaves =
	    calloc(aw->sheetsWaveCount, sizeof(WaveParam));
	if (!aw->sheetsWaves)
	{
	    compLogMessage ("animationsim", CompLogLevelError,
			    "Not enough memory");
	    return FALSE;
	}
    }
    // Compute wave parameters

    int ampDirection = (RAND_FLOAT() < 0.5 ? 1 : -1);
    int i;
    float minHalfWidth = 0.22f;
    float maxHalfWidth = 0.38f;

    for (i = 0; i < aw->sheetsWaveCount; i++)
    {
	aw->sheetsWaves[i].amp =
	    ampDirection * (waveAmpMax - waveAmpMin) *
	    rand() / RAND_MAX + ampDirection * waveAmpMin;
	aw->sheetsWaves[i].halfWidth =
	    RAND_FLOAT() * (maxHalfWidth -
			    minHalfWidth) + minHalfWidth;

	// avoid offset at top and bottom part by added waves
	float availPos = 1 - 2 * aw->sheetsWaves[i].halfWidth;
	float posInAvailSegment = 0;

	if (i > 0)
	    posInAvailSegment =
		(availPos / aw->sheetsWaveCount) * rand() / RAND_MAX;

	aw->sheetsWaves[i].pos =
	    (posInAvailSegment +
	     i * availPos / aw->sheetsWaveCount +
	     aw->sheetsWaves[i].halfWidth);

	// switch wave direction
	ampDirection *= -1;
    }

    return TRUE;
}

void
fxSheetsModelStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);

    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    ANIMSIM_WINDOW(w);

    Model *model = aw->com->model;
    XRectangle *icon = &aw->com->icon;
    CompWindow *parent;

    for (parent = w->screen->windows; parent; parent = parent->next)
    {
	if (parent->transientFor == w->id && parent->id != w->id)
	    break;
    }

    if (parent)
    {
	icon->x = WIN_X (parent) + WIN_W (parent) / 2.0f;
	icon->y = WIN_Y (parent);
	icon->width = WIN_W (w);
    }
    else
    {
	icon->x = w->screen->width / 2.0f;
	icon->y = 0.0f;
	icon->width = WIN_W (w);
    }

    float forwardProgress = (*ad->animBaseFunc->defaultAnimProgress) (w);

    if (aw->sheetsWaveCount > 0 && !aw->sheetsWaves)
	return;

    float iconCloseEndY;
    float iconFarEndY;
    float winFarEndY;
    float winVisibleCloseEndY;
    float winw = WIN_W(w);
    float winh = WIN_H(w);


    iconFarEndY = icon->y;
    iconCloseEndY = icon->y + icon->height;
    winFarEndY = WIN_Y(w) + winh;
    winVisibleCloseEndY = WIN_Y(w);
    if (winVisibleCloseEndY < iconCloseEndY)
	winVisibleCloseEndY = iconCloseEndY;


    float preShapePhaseEnd = 0.22f;
    float preShapeProgress  = 0;
    float postStretchProgress = 0;
    float stretchProgress = 0;
    float stretchPhaseEnd =
	preShapePhaseEnd + (1 - preShapePhaseEnd) *
	(iconCloseEndY -
	 winVisibleCloseEndY) / ((iconCloseEndY - winFarEndY) +
				 (iconCloseEndY - winVisibleCloseEndY));
    if (stretchPhaseEnd < preShapePhaseEnd + 0.1)
	stretchPhaseEnd = preShapePhaseEnd + 0.1;

    if (forwardProgress < preShapePhaseEnd)
    {
	preShapeProgress = forwardProgress / preShapePhaseEnd;

	// Slow down "shaping" toward the end
	preShapeProgress = 1 - (*ad->animBaseFunc->decelerateProgress) (1 - preShapeProgress);
    }

    if (forwardProgress < preShapePhaseEnd)
    {
	stretchProgress = forwardProgress / stretchPhaseEnd;
    }
    else
    {
	if (forwardProgress < stretchPhaseEnd)
	{
	    stretchProgress = forwardProgress / stretchPhaseEnd;
	}
	else
	{
	    postStretchProgress =
		(forwardProgress - stretchPhaseEnd) / (1 - stretchPhaseEnd);
	}
    }

    Object *object = model->objects;
    int i;
    for (i = 0; i < model->numObjects; i++, object++)
    {
	float origx = w->attrib.x + (winw * object->gridPosition.x -
				     w->output.left) * model->scale.x;
	float origy = w->attrib.y + (winh * object->gridPosition.y -
				     w->output.top) * model->scale.y;
	float icony = icon->y + icon->height;

	float stretchedPos;
	stretchedPos =
		object->gridPosition.y * origy +
		(1 - object->gridPosition.y) * icony;

	// Compute current y position
	if (forwardProgress < preShapePhaseEnd)
	{
	    object->position.y =
		(1 - stretchProgress) * origy +
		stretchProgress * stretchedPos;
	}
	else
	{
	    if (forwardProgress < stretchPhaseEnd)
	    {
		object->position.y =
		    (1 - stretchProgress) * origy +
		    stretchProgress * stretchedPos;
	    }
	    else
	    {
		object->position.y =
		    (1 - postStretchProgress) *
		    stretchedPos +
		    postStretchProgress *
		    (stretchedPos + (iconCloseEndY - winFarEndY));
	    }
	}

	// Compute "target shape" x position
	float yProgress = (iconCloseEndY - object->position.y ) / (iconCloseEndY - winFarEndY);

        //fprintf(stderr, "yProg is %f\n", yProgress);
	float targetx = yProgress * (origx - icon->x)
	 + icon->x + icon->width * (object->gridPosition.x - 0.5);

	// Compute current x position
	if (forwardProgress < preShapePhaseEnd)
	    object->position.x =
		(1 - preShapeProgress) * origx + preShapeProgress * targetx;
	else
	    object->position.x = targetx;

	if (object->position.y < iconFarEndY)
	    object->position.y = iconFarEndY;

	// No need to set object->position.z to 0, since they won't be used
	// due to modelAnimIs3D being FALSE for magic lamp.
    }
}
