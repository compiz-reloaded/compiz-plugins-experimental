/*
 * Animation plugin for compiz/beryl
 *
 * bonanza.c
 *
 * Copyright : (C) 2008 Kevin DuBois
 * E-mail    : kdub423@gmail.com
 *
 * Based on animations system by: (C) 2006 Erkin Bahceci
 * E-mail                       : erkinbah@gmail.com
 *
 * Based on particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                            : onestone@beryl-project.org
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
#include "animation_tex.h"

// =====================  Effect: Burn  =========================

Bool
fxBonanzaInit (CompWindow * w)
{
    CompScreen *s = w->screen;
    ANIMPLUS_DISPLAY (s->display);
    ANIMPLUS_WINDOW (w);

    if (!aw->eng->numPs)
    {
	aw->eng->ps = calloc(2, sizeof(ParticleSystem));

	aw->eng->numPs = 2;
    }
    ad->animAddonFunc->initParticles (animGetI (w, ANIMPLUS_SCREEN_OPTION_BONANZA_PARTICLES)/
		   10, &aw->eng->ps[0]);
    ad->animAddonFunc->initParticles (animGetI (w, ANIMPLUS_SCREEN_OPTION_BONANZA_PARTICLES),
		   &aw->eng->ps[1]);

    aw->eng->ps[1].slowdown = 0.5;
    aw->eng->ps[1].darken = 0.5;
    aw->eng->ps[1].blendMode = GL_ONE;

    aw->eng->ps[0].slowdown = 0.125;
    aw->eng->ps[0].darken = 0.0;
    aw->eng->ps[0].blendMode = GL_ONE_MINUS_SRC_ALPHA;

    if (!aw->eng->ps[0].tex)
	glGenTextures(1, &aw->eng->ps[0].tex);
    glBindTexture(GL_TEXTURE_2D, aw->eng->ps[0].tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, fireTex);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!aw->eng->ps[1].tex)
	glGenTextures(1, &aw->eng->ps[1].tex);
    glBindTexture(GL_TEXTURE_2D, aw->eng->ps[1].tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, fireTex);
    glBindTexture(GL_TEXTURE_2D, 0);

    aw->animFireDirection = 0;
    return TRUE;
}

static void
fxBonanzaGenFire( CompWindow *w,
                  ParticleSystem *ps,
                  int x,
                  int y,
                  int radius,
                  float size,
                  float time)
{

    float fireLife = animGetF (w, ANIMPLUS_SCREEN_OPTION_BONANZA_LIFE);
    float fireLifeNeg = 1 - fireLife;
    float fadeExtra = 0.2f * (1.01 - fireLife);
    float max_new = ps->numParticles * (time / 50) * (1.05 - fireLife);

    unsigned short *c =
	animGetC (w, ANIMPLUS_SCREEN_OPTION_BONANZA_COLOR);
    float colr1 = (float)c[0] / 0xffff;
    float colg1 = (float)c[1] / 0xffff;
    float colb1 = (float)c[2] / 0xffff;
    float colr2 = 1 / 1.7 * (float)c[0] / 0xffff;
    float colg2 = 1 / 1.7 * (float)c[1] / 0xffff;
    float colb2 = 1 / 1.7 * (float)c[2] / 0xffff;
    float cola = (float)c[3] / 0xffff;
    float rVal;

    Particle *part = ps->particles;
    int i;

    float deg = 0;
    float inc = 2.0 * 3.1415 /ps->numParticles;
    float partw = 5.00;
    float parth = partw * 1.5;
    Bool mysticalFire = animGetB (w, ANIMPLUS_SCREEN_OPTION_BONANZA_MYSTICAL);

    for (i=0; i < ps->numParticles && max_new > 0; i++, part++)
    {
        deg += inc;

        if (part->life <= 0.0f)
        {
            // give gt new life
            rVal = (float)(random() & 0xff) / 255.0;
            part->life = 1.0f;
            part->fade = rVal * fireLifeNeg + fadeExtra; // Random Fade Value

            // set size
            part->width = partw;
            part->height = parth;
            rVal = (float)(random() & 0xff) / 255.0;
            part->w_mod = part->h_mod = size * rVal;

            part->x = (float)x + (float) radius * cosf(deg);
            part->y = (float)y + (float) radius * sinf(deg);

            //clip
            if (part->x <= 0)
            part->x = 0;
            if (part->x >= 2 * x)
                part->x = 2*x;

            if (part->y <= 0)
            part->y = 0;
            if (part->y >= 2 * y)
                part->y = 2*y;

            part->z = 0.0;

            part->xo = part->x;
            part->yo = part->y;
            part->zo = 0.0f;

            // set speed and direction
            rVal = (float)(random() & 0xff) / 255.0;
            part->xi = ((rVal * 20.0) - 10.0f);
            rVal = (float)(random() & 0xff) / 255.0;
            part->yi = ((rVal * 20.0) - 15.0f);
            part->zi = 0.0f;

            if (mysticalFire)
            {
                // Random colors! (aka Mystical Fire)
                rVal = (float)(random() & 0xff) / 255.0;
                part->r = rVal;
                rVal = (float)(random() & 0xff) / 255.0;
                part->g = rVal;
                rVal = (float)(random() & 0xff) / 255.0;
                part->b = rVal;
            }
            else
            {
                rVal = (float)(random() & 0xff) / 255.0;
                part->r = colr1 - rVal * colr2;
                part->g = colg1 - rVal * colg2;
                part->b = colb1 - rVal * colb2;
            }
            // set transparancy
            part->a = cola;

            // set gravity
            part->xg = (part->x < part->xo) ? 1.0 : -1.0;
            part->yg = -3.0f;
            part->zg = 0.0f;

            ps->active = TRUE;
            max_new -= 1;
        }
        else
        {
            part->xg = (part->x < part->xo) ? 1.0 : -1.0;
        }

    }

}

void
fxBonanzaAnimStep (CompWindow *w, float time)
{
    CompScreen *s = w->screen;

    ANIMPLUS_DISPLAY (s->display);
    ANIMPLUS_WINDOW (w);

    float timestep = 8.0;
    float old = 1 - (aw->com->animRemainingTime) / (aw->com->animTotalTime - timestep);
    float stepSize;

    aw->com->animRemainingTime -= timestep;
    if (aw->com->animRemainingTime <= 0)
	    aw->com->animRemainingTime = 0;	// avoid sub-zero values
    float new = 1 - (aw->com->animRemainingTime) / (aw->com->animTotalTime - timestep);

    stepSize = new - old;

    if (aw->com->curWindowEvent == WindowEventOpen ||
	aw->com->curWindowEvent == WindowEventUnminimize ||
	aw->com->curWindowEvent == WindowEventUnshade)
    {
	new = 1 - new;
    }
    if (!aw->com->drawRegion)
	aw->com->drawRegion = XCreateRegion();


    /* define an expanding circle as a union of rectangular X regions. */
    float radius;
    if (aw->com->animRemainingTime > 0)
    {

        XRectangle rect;
        rect.width = WIN_W(w);
        rect.height = WIN_H(w);
        rect.x = WIN_X(w);
        rect.y = WIN_Y(w);

        XPoint pts[20];
        int i;
        float two_pi = 3.14159 * 2.0;
        int centerX = WIN_X(w) + WIN_W(w)/2;
        int centerY = WIN_Y(w) + WIN_H(w)/2;
        float corner_dist = sqrt( powf(WIN_H(w)/2,2) + powf(WIN_W(w)/2,2));
        radius = new * corner_dist  ;
        for (i = 0; i < 20; i++)
        {
            pts[i].x = centerX + (int)(radius * cosf( (float) i/20.0 * two_pi ));
            pts[i].y = centerY + (int)(radius * sinf( (float) i/20.0 * two_pi ));
        }

        Region r1, r2, r3;
        r3 = XCreateRegion();
        r2 = XCreateRegion();
        r1 = XCreateRegion();

        XUnionRectWithRegion(&rect, &emptyRegion, r3);
        r2 = XPolygonRegion(pts, 20, 1);

        XSubtractRegion(r3, r2, aw->com->drawRegion);


    }
    else
    {
	    XUnionRegion (&emptyRegion, &emptyRegion, aw->com->drawRegion);
	    damageScreen(w->screen);
    }


    aw->com->useDrawRegion = (fabs (new) > 1e-5);

    fxBonanzaGenFire(w,
                     &aw->eng->ps[0],
                     WIN_W(w)/2,
                     WIN_H(w)/2,
                     radius,
                     WIN_W(w) / 40.0,
                     time);

    if (aw->com->animRemainingTime <= 0 && aw->eng->numPs
	&& (aw->eng->ps[0].active ))
	{
        aw->com->animRemainingTime = 0;
    }

    if (!aw->eng->numPs || !aw->eng->ps)
    {
	    if (aw->eng->ps)
        {
            ad->animAddonFunc->finiParticles(aw->eng->ps);
            free(aw->eng->ps);
            aw->eng->ps = NULL;
        }
	// Abort animation
    fprintf(stderr, "Animation terminated\n");
	aw->com->animRemainingTime = 0;
	return;
    }
}
