/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This plugin renders a fish tank inside of the transparent cube,
 * replete with fish, crabs, sand, bubbles, and coral.
 *
 * Copyright : (C) 2007-2008 by David Mikos
 * Email     : infiniteloopcounter@gmail.com
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
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
 */

/*
 * Bubble movement code by David Mikos.
 */

#include "atlantis-internal.h"
#include <math.h>
#include <float.h>
#include "atlantis_options.h"

void
BubbleTransform(Bubble * bubble)
{
    glTranslatef (bubble->y, bubble->z, bubble->x);
}

void
BubblePilot(CompScreen * s,
            int aeratorIndex,
            int bubbleIndex)
{
    ATLANTIS_SCREEN (s);

    int i;

    Bubble * bubble = &(as->aerator[aeratorIndex].bubbles[bubbleIndex]);

    float x = bubble->x;
    float y = bubble->y;
    float z = bubble->z;

    float top = (atlantisGetRenderWaves (s) ? 100000*
		 getHeight(as->water, x / (100000 * as->ratio),
		           y / (100000 * as->ratio)) : as->waterHeight);

    float perpDist = (as->sideDistance - bubble->size);
    float tempAng;
    float dist;


    z += as->speedFactor * bubble->speed;

    if (z > top - 2 * bubble->size)
    {
	x = as->aerator[aeratorIndex].x;
	y = as->aerator[aeratorIndex].y;
	z = as->aerator[aeratorIndex].z;
	bubble->speed   = 100 + randf (150);
	bubble->offset  = randf (2 * PI);
	bubble->counter = 0;
    }
    bubble->counter++;

    tempAng = fmodf (0.1 * bubble->counter * as->speedFactor + bubble->offset,
                     2 * PI);
    x += 50 * sinf (tempAng);
    y += 50 * cosf (tempAng);

    tempAng = atan2f (y, x);
    dist    = hypotf (x, y);

    for (i = 0; i < as->hsize; i++)
    {
	float directDist;
	float cosAng = cosf (fmodf (i * as->arcAngle * toRadians - tempAng,
	                            2 * PI));
	if (cosAng <= 0)
	    continue;

	directDist = perpDist / cosAng;

	if (dist > directDist)
	{
	    x = directDist * cosf (tempAng);
	    y = directDist * sinf (tempAng);
	    tempAng = atan2f (y, x);
	    dist    = hypotf (x, y);
	}
    }

    bubble->x = x;
    bubble->y = y;
    bubble->z = z;
}
