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
 * Crab movement code by David Mikos.
 */

#include "atlantis-internal.h"
#include <math.h>
#include <float.h> /* for FLT_MAX */


void
CrabTransform (crabRec * crab)
{
    glTranslatef (crab->y, crab->z, crab->x);
    glRotatef (crab->psi+180, 0.0, 1.0, 0.0);
    glRotatef (crab->theta, 1.0, 0.0, 0.0);
}

void
CrabPilot (CompScreen *s, int index)
{
    ATLANTIS_SCREEN (s);

    int i;

    crabRec * crab = &(as->crab[index]);

    float moveAmount = 0; /* for falling movements/scuttling in proper ratios
			     (1 when fully moved) */

    float maxVel = crab->speed;

    float x = crab->x;
    float y = crab->y;
    float z = crab->z;

    float bottom = getGroundHeight (s, x, y);

    if (z > bottom) { /* falling */
	float fallDist = as->speedFactor * crab->size / 5;
	z -= fallDist;

	if (z <= bottom)
	{
	    if (crab->isFalling)
		moveAmount = (crab->z - z) / fallDist;
	    z = bottom;
	    crab->isFalling = FALSE;
	}
	else
	{
	    crab->scuttleAmount = 0;
	    crab->isFalling = TRUE;
	}
    }

    if (!crab->isFalling && moveAmount <= 1)
    { /* walking along a surface */
	float factor = as->speedFactor * (1 - moveAmount);
	float ang, temp;

	if (crab->scuttleAmount <= 0)
	{
	    crab->speed = 1 + randf (200);
	    temp = 20 / (sqrtf (crab->speed));
	    crab->scuttlePsi = randf (2 * temp) - temp;
	    if (NRAND (2) == 0) crab->speed *= -1;

	    crab->scuttleTheta= 0;
	    crab->scuttleAmount = (int) ((randf (30) + 7) / as->speedFactor);
	    if (crab->scuttleAmount < 1) crab->scuttleAmount = 1;
	}

	crab->scuttleAmount--;

	crab->psi   += factor * crab->scuttlePsi;
	crab->theta += factor * crab->scuttleTheta;

	crab->psi   = fmodf (crab->psi,   360);
	crab->theta = fmodf (crab->theta, 360);


	factor *= maxVel;
	x += factor * cosf (crab->psi *toRadians) *
	     cosf (crab->theta *toRadians);
	y += factor * sinf (crab->psi *toRadians) *
	     cosf (crab->theta *toRadians);
	//crab->z += factor * sinf (crab->theta *toRadians);

	ang = atan2f (y, x);

	for (i = 0; i < as->hsize; i++)
	{
	    float cosAng = cosf (fmodf (i * as->arcAngle * toRadians - ang,
	                                2 * PI));
	    if (cosAng <= 0)
		continue;

	    float d = (as->sideDistance - 0.75 * crab->size) / cosAng;
	    float d2 = hypotf (x, y);

	    if (d2 > d)
	    {
		x = d * cosf (ang);
		y = d * sinf (ang);
	    }

	}
	z = getGroundHeight (s, x, y);
    }
    crab->x = x;
    crab->y = y;

    if (z < bottom)
	crab->z = bottom;
    else
	crab->z = z;
}
