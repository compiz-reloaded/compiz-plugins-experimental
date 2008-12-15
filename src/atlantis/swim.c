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
 * swim.c written by David Mikos.
 */

/*
 * Based on swim.c by Dennis Kasprzyk.
 */

/*
 * In turn based on atlantis xscreensaver http://www.jwz.org/xscreensaver/
 */

/* Copyright (c) E. Lassauge, 1998. */

/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * The original code for this mode was written by Mark J. Kilgard
 * as a demo for openGL programming.
 *
 * Porting it to xlock  was possible by comparing the original Mesa's morph3d
 * demo with it's ported version to xlock, so thanks for Marcelo F. Vianna
 * (look at morph3d.c) for his indirect help.
 *
 * Thanks goes also to Brian Paul for making it possible and inexpensive
 * to use OpenGL at home.
 *
 * My e-mail address is lassauge@users.sourceforge.net
 *
 * Eric Lassauge  (May-13-1998)
 *
 */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include "atlantis-internal.h"
#include "atlantis_options.h"
#include <math.h>
#include <float.h>

static float boidsPsi;   /* the psi   angle that a fish should turn when
			    only applying boids. */
static float boidsTheta; /* the theta angle that a fish should turn when
			    only applying boids. */

void
FishTransform (fishRec * fish)
{
    glTranslatef (fish->y, fish->z, fish->x);
    glRotatef (fish->psi + 180, 0.0, 1.0, 0.0);
    glRotatef (fish->theta,     1.0, 0.0, 0.0);
}

void
FishPilot (CompScreen *s,
           int index)
{
    ATLANTIS_SCREEN (s);

    fishRec * fish = &(as->fish[index]);

    int i, j;

    float speed = fish->speed;
    float x = fish->x;
    float y = fish->y;
    float z = fish->z;

    float maxVel = as->speedFactor * speed;
    float maxTurnAng = 15 * as->speedFactor; /* left/right angle (in degrees) */
    float maxTurnTh = 5 * as->speedFactor; /* up/down angle (in degrees) */

    //float minOppTurnAng[2]= { -maxTurnAng, -maxTurnAng };
    float minOppTurnAng[2]= { 0, 0 };

    float ang; /* angle of the fish so that 90-ang is the turn remaining
		  for each wall */
    float tTemp[as->hsize]; /* distance to wall when not changing angle */

    float tempAng = atan2f(y, x);
    float dist = hypotf(x, y);
    float perpDist = (as->sideDistance - fish->size/2);

    float t = FLT_MAX;
    int iWall = 0; /* The number of the wall most in the way of the fish */

    float distRem, cosAng;
    float mota; /* MIN opposite turn angle */

    float turnRem[2]; /* right/left */
    float sign[2] = { 1, -1 };
    float top[2], bottom[2];
    float Q1;

    float turnAng = 0, turnTh = 0;
    float prefTurnAng, prefTurnTh;

    fish->psi = fmodf(fish->psi, 360);
    if (fish->psi>180)
	fish->psi-=360;
    else if (fish->psi<-180)
	fish->psi+=360;

    if (as->waterHeight-fish->size/2-z<0 && fish->type!=DOLPHIN)
    { /* dolphins are allowed to jump out top briefly */
	fish->z = as->waterHeight-fish->size/2;
    }
    float bottomTank = getGroundHeight(s, x, y);

    if (z-bottomTank-fish->size/2<0)
    { /* stop all fish from going too far down */
	fish->z = bottomTank+fish->size/2;
    }

    for (i=0; i<as->hsize; i++)
    {
	float cosAng = cosf (fmodf (i * as->arcAngle * toRadians - tempAng,
	                            2 * PI));

	tTemp[i] = (perpDist - dist * cosAng) / cosf (
	           fmodf (fish->psi * toRadians - i * as->arcAngle * toRadians,
	                  2*PI));

	if (cosAng > 0)
	{
	    float d = dist * cosAng - perpDist;

	    if (d > 0) {
		x -= d * cosf (tempAng) *
		     fabsf (cosf (i * as->arcAngle * toRadians));
		y -= d * sinf (tempAng) *
		     fabsf (sinf (i * as->arcAngle * toRadians));
		fish->x = x;
		fish->y = y;
		tTemp[i] = 0.1;
		tempAng = atan2f (y, x);
		dist = hypotf (x, y);
	    }
	}

	if ((tTemp[i] < t && tTemp[i] >= 0))
	{
	    t = tTemp[i];
	    iWall = i;
	}
    }

    /* need to consider walls iWall, iWall+1 for clockwise iWall, iWall-1 for anticlockwise.
       want the angle that the fish can turn minimally right/left such that the fish can
       still dodge the wall smoothly in the next frame. */

    /* turnRem should be 90 degrees if heading straight at a wall, etc.
       distRem shoud be positive if inside aquarium and negative if outside. */

    ang = fish->psi - iWall * as->arcAngle;

    for (j = 0; j < 2; j++)
    {
	float signAng = sign[j] * ang;
	i = iWall;

	cosAng = cosf (fmodf (iWall * as->arcAngle * toRadians - tempAng, 2 * PI));
	distRem = fabsf (perpDist - dist * cosAng);

	turnRem[j] = fmodf(90 - signAng, 360);
	if (turnRem[j] < 0)
	    turnRem[j] += 360;

	top[j] = sinf ((signAng + turnRem[j]) * toRadians) - sinf (signAng * toRadians);
	bottom[j] = 2 * distRem / maxVel + cosf (signAng * toRadians) -
		    cosf ((signAng + turnRem[j]) * toRadians);

	Q1 = 2 * atan2f (top[j], bottom[j]) * toDegrees;
	Q1 = fmodf(Q1, 360);
	if (Q1 < 0)
	    Q1 += 360.0f;

	mota = turnRem[j] - maxTurnAng * (turnRem[j] / Q1 - 1);
	if (turnRem[j] / Q1 <= 1 && turnRem[j] / Q1 >= 0)
	    mota = Q1;

	if (mota > minOppTurnAng[j] && mota < 300)
	    minOppTurnAng[j] = mota;

	signAng -= sign[j] * as->arcAngle;
	turnRem[j] = fmodf (90 - signAng, 360);
	if (turnRem[j] < 0)
	    turnRem[j] += 360;

	if (j == 0)
	    i = (iWall + 1) % as->hsize;
	else
	    i = (iWall + as->hsize - 1) % as->hsize;

	cosAng = cosf (fmodf (i * as->arcAngle * toRadians - tempAng, 2 * PI));
	distRem = fabsf (perpDist - dist * cosAng);

	top[j] = sinf ((signAng + turnRem[j]) * toRadians) - sinf (signAng * toRadians);
	bottom[j] = 2 * distRem / maxVel + cosf (signAng * toRadians) -
		    cosf ((signAng + turnRem[j]) * toRadians);

	Q1 = 2 * atan2f (top[j], bottom[j]) * toDegrees;
	Q1 = fmodf (Q1, 360);
	if (Q1 < 0)
	    Q1 += 360.0f;

	mota = turnRem[j] - maxTurnAng * (turnRem[j] / Q1 - 1);
	if (turnRem[j] / Q1 <= 1 && turnRem[j] / Q1 >= 0)
	    mota = Q1;

	if (mota > minOppTurnAng[j])
	    minOppTurnAng[j] = mota;
    }

    if (fish->boidsCounter <= 0)
    { /* update the boidsAngles */
	BoidsAngle (s, index);

	fish->boidsCounter = 2 + NUM_GROUPS / as->speedFactor;
    }
    fish->boidsCounter--;

    boidsPsi   = fish->boidsPsi;
    boidsTheta = fish->boidsTheta;

    prefTurnTh = fmodf (boidsTheta - fish->theta, 360);
    if (prefTurnTh > 180)
	prefTurnTh -= 360;
    if (prefTurnTh <- 180)
	prefTurnTh += 360;

    if (fish->smoothTurnCounter <= 0)
    {
	float divideAmount;

	prefTurnAng = fmodf(boidsPsi - fish->psi, 360); /* boids turn angle*/
	if (prefTurnAng>180)
	    prefTurnAng-=360;
	if (prefTurnAng<-180)
	    prefTurnAng+=360;

	fish->smoothTurnCounter = 5 + NRAND( (int) (20.0 / as->speedFactor));
	fish->smoothTurnAmount = turnAng;

	switch (fish->type)
	{
	    case SHARK:
		divideAmount = 5 + 10000 / fish->size +
			       randf (10000 / fish->size + 8);
		break;
	    case DOLPHIN:
		divideAmount = 4 + 10000 / fish->size +
			       randf (10000 / fish->size + 5);
		break;

	    default:
		divideAmount = 1 + 10000 / fish->size +
			       randf(1.2 * 10000 / fish->size + 5);
		break;
	}
	if (divideAmount <= 1)
	    divideAmount = 1;

	fish->smoothTurnCounter = (int) 1 + fabsf (prefTurnAng /
	                          (divideAmount * as->speedFactor / 2));
	turnAng = prefTurnAng / ((float) fish->smoothTurnCounter);
	fish->smoothTurnAmount = turnAng;

	turnTh = prefTurnTh / ((float) fish->smoothTurnCounter);
	fish->smoothTurnTh = turnTh;
    }
    else
    {
	prefTurnAng = fish->smoothTurnAmount;
	turnAng = prefTurnAng;

	prefTurnAng = fish->smoothTurnTh;
	turnTh = prefTurnTh;

	if (minOppTurnAng[0] > prefTurnAng && -minOppTurnAng[0] < prefTurnAng)
	{
	    if (fabsf ( minOppTurnAng[0] - prefTurnAng) <
		fabsf (-minOppTurnAng[1] - prefTurnAng))
		turnAng = minOppTurnAng[0];
	    else
		turnAng = -minOppTurnAng[1];
	}
    }
    fish->smoothTurnCounter--;

    fish->speed += randf(40) - 20; /* new speed */
    if (fish->speed > 200)
	fish->speed = 200;
    if (fish->speed < 50)
	fish->speed = 50;

    if (turnAng > maxTurnAng)
	fish->psi += maxTurnAng;
    else if (turnAng < -maxTurnAng)
	fish->psi -= maxTurnAng;
    else
	fish->psi += turnAng;

    fish->psi = fmodf (fish->psi, 360);

    if (fish->psi > 180)
	fish->psi -= 360;
    else if (fish->psi <- 180)
	fish->psi += 360;

    if (turnTh > maxTurnTh)
	fish->theta += maxTurnTh;
    else if (turnTh <- maxTurnTh)
	fish->theta -= maxTurnTh;
    else
	fish->theta += turnTh;

    if (fish->theta > 50)
	fish->theta = 50;
    else if (fish->theta <- 50)
	fish->theta =- 50;

    fish->x += maxVel * cosf (fish->psi * toRadians) *
	       cosf (fish->theta * toRadians);
    fish->y += maxVel * sinf (fish->psi * toRadians) *
	       cosf (fish->theta * toRadians);
    fish->z += maxVel * sinf (fish->theta * toRadians);
}

void
BoidsAngle(CompScreen *s,
           int i)
{
    ATLANTIS_SCREEN (s);

    float x = as->fish[i].x;
    float y = as->fish[i].y;
    float z = as->fish[i].z;

    float psi = as->fish[i].psi;
    float theta = as->fish[i].theta;

    int type = as->fish[i].type;

    float factor = 5+5*fabsf(symmDistr());
    float randPsi = 10*symmDistr();
    float randTh = 10*symmDistr();

    /* (X,Y,Z) is the boids vector */
    float X = factor * cosf ((psi + randPsi) * toRadians) *
	      cosf ((theta + randTh) * toRadians) / 50000;
    float Y = factor * sinf ((psi + randPsi) * toRadians) *
	      cosf ((theta + randTh) * toRadians) / 50000;
    float Z = factor * sinf ((theta + randTh) * toRadians) / 50000;

    float tempAng = atan2f (y, x);
    float dist = hypotf (x, y);

    float perpDist;
    int j;

    for (j = 0; j < as->hsize; j++)
    { /* consider side walls */
	float wTheta = j * as->arcAngle*toRadians;

	float cosAng = cosf (fmodf (j * as->arcAngle * toRadians - tempAng,
	                            2 * PI));
	perpDist = fabsf (as->sideDistance - as->fish[i].size / 2 -
	                  dist * cosAng);

	if (perpDist > 50000)
	    continue;

	if (perpDist <= as->fish[i].size / 2)
	    perpDist  = as->fish[i].size / 2;

	factor = 1 / ((float) as->hsize);
	if (perpDist <= as->fish[i].size)
	    factor *= as->fish[i].size / perpDist;

	X -= factor * cosf (wTheta) / perpDist;
	Y -= factor * sinf (wTheta) / perpDist;
    }

    perpDist = as->waterHeight - z; /* top wall */
    if (perpDist <= as->fish[i].size / 2)
	perpDist = as->fish[i].size / 2;
    factor = 1;
    if (perpDist <= as->fish[i].size)
	factor = as->fish[i].size / perpDist;
    Z -= factor / perpDist;

    perpDist = z - getGroundHeight(s, x, y); /* bottom wall */
    if (perpDist <= as->fish[i].size / 2)
	perpDist  = as->fish[i].size / 2;
    factor = 1;
    if (perpDist <= as->fish[i].size)
	factor = as->fish[i].size / perpDist;
    Z += factor / perpDist;

    for (j = 0; j < as->numFish; j++)
    { /* consider other fish */
	if (j != i)
	{
	    factor = 1; /* positive means form group, negative means stay away.
			   the amount is proportional to the relative
			   importance of the pairs of fish.*/
	    if (type < as->fish[j].type)
	    {
		if (as->fish[j].type <= FISH2)
		    factor =-1; /* fish is coming up against different fish */
		else
		    factor = (float) (type - as->fish[j].type) * 3;
		    /* fish is coming against a shark, etc. */
	    }
	    else if (type == as->fish[j].type)
	    {
		if (as->fish[i].group != as->fish[j].group &&
		    !atlantisGetSchoolSimilarGroups(s))
		    factor =-1; /* fish is coming up against different fish */
	    }
	    else
		continue; /* whales are not bothered,
			     sharks are not bothered by fish, etc. */

	    if (atlantisGetSchoolSimilarGroups(s))
	    {
		if ( (type == CHROMIS  && (as->fish[j].type == CHROMIS2 ||
					   as->fish[j].type == CHROMIS3)) ||
		     (type == CHROMIS2 && (as->fish[j].type == CHROMIS  ||
					   as->fish[j].type == CHROMIS3)) ||
		     (type == CHROMIS3 && (as->fish[j].type == CHROMIS  ||
					   as->fish[j].type == CHROMIS2)))
		    factor = 1;
	    }

	    float xt = (as->fish[j].x - x);
	    float yt = (as->fish[j].y - y);
	    float zt = (as->fish[j].z - z);
	    float d = sqrtf (xt * xt + yt * yt + zt * zt);

	    float th = fmodf (atan2f (yt, xt) * toDegrees - psi, 360);
	    if (th > 180)
		th -= 360;
	    if (th <- 180)
		th += 360;

	    if (fabsf (th) < 80 &&
		fabsf (asinf (zt / d) * toDegrees - theta ) < 80)
	    { /* in field of view of fish */

		th = fmodf(as->fish[j].psi - psi, 360);
		if (th <- 180)
		    th += 360;
		if (th > 180)
		    th -= 360;

		if (factor>0 && (fabsf (th) > 90 ||
				 fabsf (as->fish[j].theta - theta) < 90))
		{
		    /* other friendly fish heading in near opposite direction
		       the idea is to turn to form a group by taking the lead
		       or sneaking behind */

		    if (d > 50000 / 2)
		    { /* varies as distance to power [1,2] after
			 this distance */
			d = powf(d, 1 + (d - 50000 / 2) /
			         (2 * 50000 - 50000 / 2));
		    }

		    factor /= d;
		    X += factor * cosf (as->fish[j].psi * toRadians) *
			 cosf (as->fish[j].theta * toRadians);
		    Y += factor * sinf (as->fish[j].psi * toRadians) *
			 cosf (as->fish[j].theta * toRadians);
		    Z += factor * sinf (as->fish[j].theta * toRadians);
		}
		else
		{
		    if (d > 50000 / 2)
		    { /* varies as distance to power [1,2] after
			 this distance */
			d = powf (d, 2 + (d - 50000 / 2) /
				  (2 * 50000 - 50000 / 2));
		    }
		    else
		    { /* varies as distance */
			d *= d;
		    }

		    /* note an extra factor of d due to
		       normalizing (xt, yt, zt) */

		    factor /= d;
		    X += factor * xt;
		    Y += factor * yt;
		    Z += factor * zt;
		}
	    }
	}
    }

    as->fish[i].boidsPsi = atan2f (Y, X) * toDegrees;
    /* needs fmoding, etc to get into range [-180,180]
       angle not relative to fish angle
       (have to minus as->fish.psi when using this angle) */

    if (isnan (as->fish[i].boidsPsi))
	as->fish[i].boidsPsi = psi; /* precaution */

    as->fish[i].boidsTheta = asinf (Z / sqrtf (X * X + Y * Y + Z * Z)) *
			     toDegrees;
    if (isnan (as->fish[i].boidsTheta))
	as->fish[i].boidsTheta = theta; /* precaution */
}
