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
 * E-mail    : onestone@compiz.org
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


#include "atlantis-internal.h"
#include <math.h>

void
DrawBubble (int wire,
            int nStrips)
{
    int horizStrips = nStrips;
    int vertStrips  = nStrips;

    float ang;
    float topSinAng, topCosAng;
    float bottomSinAng, bottomCosAng;

    int i, j;

    float x, y;

    for (i = 0; i < horizStrips; i++)
    {
	ang = i * PI / horizStrips;
	topSinAng = sinf (ang);
	topCosAng = cosf (ang);

	ang -= PI / horizStrips;
	bottomSinAng = sinf (ang);
	bottomCosAng = cosf (ang);

	glBegin (wire ? GL_LINE_LOOP : GL_QUAD_STRIP);

	for (j = -1; j < vertStrips; j++)
	{
	    ang = j * 2 * PI / vertStrips;

	    x = cosf (ang);
	    y = sinf (ang);

	    glNormal3f (y * bottomSinAng, bottomCosAng,     x * bottomSinAng);
	    glVertex3f (y * bottomSinAng, bottomCosAng + 1, x * bottomSinAng);
	    glNormal3f (y * topSinAng,    topCosAng,        x * topSinAng);
	    glVertex3f (y * topSinAng,    topCosAng + 1,    x * topSinAng);
	}

	glEnd();
    }
}
