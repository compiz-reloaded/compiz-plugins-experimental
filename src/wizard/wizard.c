/*
 * Compiz wizard particle system plugin
 *
 * wizard.c
 *
 * Written by : Sebastian Kuhlen
 * E-mail     : DiCon@tankwar.de
 *
 * This plugin and parts of its code have been inspired by the showmouse plugin
 * by Dennis Kasprzyk
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

#include <math.h>
#include <string.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>

#include "wizard_options.h"
#include "wizard_tex.h"

#define GET_WIZARD_DISPLAY(d)                                  \
    ((WizardDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define WIZARD_DISPLAY(d)                      \
    WizardDisplay *wd = GET_WIZARD_DISPLAY (d)

#define GET_WIZARD_SCREEN(s, sd)                                   \
    ((WizardScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)

#define WIZARD_SCREEN(s)                                                      \
    WizardScreen *ws = GET_WIZARD_SCREEN (s, GET_WIZARD_DISPLAY (s->display))

static float
rRange (float avg, float range)
{
    return avg + (float)((random () & 0xff)/127.5-1.)*range;
}

typedef enum
{
    TRIGGER_PERSISTENT = 0,
    TRIGGER_MOUSEMOVEMENT,
    TRIGGER_RANDOMSHOT,
    TRIGGER_RANDOMPERIOD
} TriggerType;

typedef enum
{
    MOVEMENT_MOUSEPOSITION = 0,
    MOVEMENT_FOLLOWMOUSE,
    MOVEMENT_BOUNCE,
    MOVEMENT_WRAP
} MovementType;

typedef struct _GPoint
{
    float strength;		// Strength of this gravity source
    float x;			// X position
    float y;			// Y position
    float espeed;		// Speed of the gravity source
    float eangle;		// Angle for the movement of this gravity source
    int   movement;		// Type of movement of this gravity source
} GPoint;

typedef struct _Particle
{
    float c[3];			// Color
    float a;			// alpha value
    float x;			// X position
    float y;			// Y position
    float t;			// t position (age, born at 1, dies at 0)
    float phi;			// Orientation of texture
    float vx;			// X speed
    float vy;			// Y speed
    float vt;			// t speed (aging speed)
    float vphi;			// Rotation speed
    float s;			// size (side of the square)
    float snew;			// Size when born (reduced to s while new)
    float g;			// Gravity from this particle
} Particle;

typedef struct _Emitter
{
    Bool  set_active;		// Set to active in the settings
    Bool  active;		// Currently active (differs from set_active for
				//                   the random period trigger)
    int   trigger;		// When to generate particles
    int   count;		// Amount of particles to be generated
    float h;			// color hue (0..1)
    float dh;			// color hue range
    float l;			// color lightness (0..1)
    float dl;			// color lightness range
    float a;			// Alpha
    float da;			// Alpha range
    float x;			// X position
    float y;			// Y position
    float espeed;		// Speed of the emitter
    float eangle;		// Angle for the movement of this emitter
    int   movement;		// Type of movement of this emitter
    float dx;			// X range
    float dy;			// Y range
    float dcirc;		// Circular range
    float vx;			// X speed
    float vy;			// Y speed
    float vt;			// t speed (aging speed)
    float vphi;			// Rotation speed
    float dvx;			// X speed range
    float dvy;			// Y speed range
    float dvcirc;		// Circular speed range
    float dvt;			// t speed (aging speed) range
    float dvphi;		// Rotation speed range
    float s;			// size (side of the square)
    float ds;			// size (side of the square) range
    float snew;			// Size when born (reduced to s while new)
    float dsnew;		// Size when born (reduced to s while new) range
    float g;			// Gravity of particles
    float dg;			// Gravity range
    float gp;			// Part of particles that have gravity
} Emitter;

typedef struct _ParticleSystem
{
    int      hardLimit;		// Not to be exceeded
    int      softLimit;		// If exceeded, old particles age faster
    int      lastCount;		// Living particle count to evaluate softLimit
    float    tnew;		// Particle is new if t > tnew
    float    told;		// Particle is old if t < told
    float    gx;		// Global gravity x
    float    gy;		// Global gravity y
    Particle *particles;	// The actual particles
    GLuint   tex;		// Particle Texture
    Bool     active;
    float    darken;		// Darken background
    GLuint   blendMode;
    Emitter  *e;		// All emitters in here
    GPoint   *g;		// All gravity point sources in here
    int      ne;		// Emitter count
    int      ng;		// GPoint count

    GLfloat *vertices_cache;
    int     vertex_cache_count;
    GLfloat *coords_cache;
    int     coords_cache_count;
    GLfloat *colors_cache;
    int     color_cache_count;
    GLfloat *dcolors_cache;
    int     dcolors_cache_count;
} ParticleSystem;

static int displayPrivateIndex = 0;

typedef struct _WizardDisplay
{
    int screenPrivateIndex;
    MousePollFunc *mpFunc;
} WizardDisplay;

typedef struct _WizardScreen
{
    int mx, my; //Mouse Position from polling

    Bool active;

    ParticleSystem *ps;

    PositionPollingHandle pollHandle;

    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc        paintOutput;
} WizardScreen;

static void
initParticles (int hardLimit, int softLimit, ParticleSystem * ps)
{
    if (ps->particles)
	free (ps->particles);
    ps->particles    = calloc (hardLimit, sizeof (Particle));
    ps->tex          = 0;
    ps->hardLimit    = hardLimit;
    ps->softLimit    = softLimit;
    ps->active       = FALSE;
    ps->lastCount    = 0;

    // Initialize cache
    ps->vertices_cache      = NULL;
    ps->colors_cache        = NULL;
    ps->coords_cache        = NULL;
    ps->dcolors_cache       = NULL;
    ps->vertex_cache_count  = 0;
    ps->color_cache_count   = 0;
    ps->coords_cache_count  = 0;
    ps->dcolors_cache_count = 0;

    Particle *part = ps->particles;
    int i;
    for (i = 0; i < hardLimit; i++, part++)
	part->t = 0.0f;
}

static void
loadGPoints (CompScreen *s, ParticleSystem *ps)
{
    if (ps->g)
	free (ps->g);

    int i;
    GPoint *gi;
    CompListValue *clv = wizardGetGStrength (s);
    ps->ng = clv->nValue;
    ps->g = calloc (ps->ng, sizeof (GPoint));

    gi = ps->g;
    for (i = 0; i < ps->ng; i++, gi++)
	gi->strength = (float)clv->value[i].i / 1000.;

    clv = wizardGetGPosx (s);
    gi = ps->g;
    for (i = 0; i < ps->ng; i++, gi++)
	gi->x = (float)clv->value[i].i;

    clv = wizardGetGPosy (s);
    gi = ps->g;
    for (i = 0; i < ps->ng; i++, gi++)
	gi->y = (float)clv->value[i].i;

    clv = wizardGetGSpeed (s);
    gi = ps->g;
    for (i = 0; i < ps->ng; i++, gi++)
	gi->espeed = (float)clv->value[i].i / 100.;

    clv = wizardGetGAngle (s);
    gi = ps->g;
    for (i = 0; i < ps->ng; i++, gi++)
	gi->eangle = (float)clv->value[i].i / 180.*M_PI;

    clv = wizardGetGMovement (s);
    gi = ps->g;
    for (i = 0; i < ps->ng; i++, gi++)
	gi->movement = clv->value[i].i;
}

static void
loadEmitters (CompScreen *s, ParticleSystem *ps)
{
    if (ps->e)
	free (ps->e);

    int i;
    Emitter *ei;
    CompListValue *clv = wizardGetEActive (s);
    ps->ne = clv->nValue;
    ps->e = calloc (ps->ne, sizeof (Emitter));

    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->set_active = ei->active = clv->value[i].b;

    clv = wizardGetETrigger (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->trigger = clv->value[i].i;

    clv = wizardGetEPosx (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->x = (float)clv->value[i].i;

    clv = wizardGetEPosy (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->y = (float)clv->value[i].i;

    clv = wizardGetESpeed (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->espeed = (float)clv->value[i].i / 100.;

    clv = wizardGetEAngle (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->eangle = (float)clv->value[i].i / 180.*M_PI;

    clv = wizardGetEMovement (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->movement = clv->value[i].i;

    clv = wizardGetECount (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->count = (float)clv->value[i].i;

    clv = wizardGetEH (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->h = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDh (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dh = (float)clv->value[i].i / 1000.;

    clv = wizardGetEL (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->l = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDl (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dl = (float)clv->value[i].i / 1000.;

    clv = wizardGetEA (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->a = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDa (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->da = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDx (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dx = (float)clv->value[i].i;

    clv = wizardGetEDy (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dy = (float)clv->value[i].i;

    clv = wizardGetEDcirc (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dcirc = (float)clv->value[i].i;

    clv = wizardGetEVx (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->vx = (float)clv->value[i].i / 1000.;

    clv = wizardGetEVy (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->vy = (float)clv->value[i].i / 1000.;

    clv = wizardGetEVt (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->vt = (float)clv->value[i].i / 10000.;

    clv = wizardGetEVphi (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->vphi = (float)clv->value[i].i / 10000.;

    clv = wizardGetEDvx (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dvx = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDvy (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dvy = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDvcirc (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dvcirc = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDvt (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dvt = (float)clv->value[i].i / 10000.;

    clv = wizardGetEDvphi (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dvphi = (float)clv->value[i].i / 10000.;

    clv = wizardGetES (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->s = (float)clv->value[i].i;

    clv = wizardGetEDs (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->ds = (float)clv->value[i].i;

    clv = wizardGetESnew (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->snew = (float)clv->value[i].i;

    clv = wizardGetEDsnew (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dsnew = (float)clv->value[i].i;

    clv = wizardGetEG (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->g = (float)clv->value[i].i / 1000.;

    clv = wizardGetEDg (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->dg = (float)clv->value[i].i / 1000.;

    clv = wizardGetEGp (s);
    ei = ps->e;
    for (i = 0; i < ps->ne; i++, ei++)
	ei->gp = (float)clv->value[i].i / 10000.;
}

static void
drawParticles (CompScreen * s, ParticleSystem * ps)
{
    glPushMatrix ();

    glEnable (GL_BLEND);
    if (ps->tex)
    {
	glBindTexture (GL_TEXTURE_2D, ps->tex);
	glEnable (GL_TEXTURE_2D);
    }
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    /* Check that the cache is big enough */
    if (ps->hardLimit > ps->vertex_cache_count)
    {
	ps->vertices_cache =
	    realloc (ps->vertices_cache,
		    ps->hardLimit * 4 * 3 * sizeof (GLfloat));
	ps->vertex_cache_count = ps->hardLimit;
    }

    if (ps->hardLimit > ps->coords_cache_count)
    {
	ps->coords_cache =
	    realloc (ps->coords_cache,
		    ps->hardLimit * 4 * 2 * sizeof (GLfloat));
	ps->coords_cache_count = ps->hardLimit;
    }

    if (ps->hardLimit > ps->color_cache_count)
    {
	ps->colors_cache =
	    realloc (ps->colors_cache,
		    ps->hardLimit * 4 * 4 * sizeof (GLfloat));
	ps->color_cache_count = ps->hardLimit;
    }

    if (ps->darken > 0)
    {
	if (ps->dcolors_cache_count < ps->hardLimit)
	{
	    ps->dcolors_cache =
		realloc (ps->dcolors_cache,
			ps->hardLimit * 4 * 4 * sizeof (GLfloat));
	    ps->dcolors_cache_count = ps->hardLimit;
	}
    }

    GLfloat *dcolors  = ps->dcolors_cache;
    GLfloat *vertices = ps->vertices_cache;
    GLfloat *coords   = ps->coords_cache;
    GLfloat *colors   = ps->colors_cache;

    int cornersSize = sizeof (GLfloat) * 8;
    int colorSize   = sizeof (GLfloat) * 4;

    GLfloat cornerCoords[8] = {0.0, 0.0,
			       0.0, 1.0,
			       1.0, 1.0,
			       1.0, 0.0};

    int numActive = 0;

    Particle *part = ps->particles;
    int i;
    for (i = 0; i < ps->hardLimit; i++, part++)
    {
	if (part->t > 0.0f)
	{
	    numActive += 4;

	    float cOff = part->s / 2.;		//Corner offset from center

	    if (part->t > ps->tnew)		//New particles start larger
		cOff += (part->snew - part->s) * (part->t - ps->tnew)
			/ (1. - ps->tnew) / 2.;
	    else if (part->t < ps->told)	//Old particles shrink
		cOff -= part->s * (ps->told - part->t) / ps->told / 2.;

	    //Offsets after rotation of Texture
	    float offA = cOff * (cos (part->phi) - sin (part->phi));
	    float offB = cOff * (cos (part->phi) + sin (part->phi));

	    vertices[0] = part->x - offB;
	    vertices[1] = part->y - offA;
	    vertices[2] = 0;

	    vertices[3] = part->x - offA;
	    vertices[4] = part->y + offB;
	    vertices[5] = 0;

	    vertices[6] = part->x + offB;
	    vertices[7] = part->y + offA;
	    vertices[8] = 0;

	    vertices[9]  = part->x + offA;
	    vertices[10] = part->y - offB;
	    vertices[11] = 0;

	    vertices += 12;

	    memcpy (coords, cornerCoords, cornersSize);

	    coords += 8;

	    colors[0] = part->c[0];
	    colors[1] = part->c[1];
	    colors[2] = part->c[2];

	    if (part->t > ps->tnew)		//New particles start at a == 1
		colors[3] = part->a + (1. - part->a) * (part->t - ps->tnew)
					/ (1. - ps->tnew);
	    else if (part->t < ps->told)	//Old particles fade to a = 0
		colors[3] = part->a * part->t / ps->told;
	    else				//The others have their own a
		colors[3] = part->a;

	    memcpy (colors + 4, colors, colorSize);
	    memcpy (colors + 8, colors, colorSize);
	    memcpy (colors + 12, colors, colorSize);

	    colors += 16;

	    if (ps->darken > 0)
	    {
		dcolors[0] = colors[0];
		dcolors[1] = colors[1];
		dcolors[2] = colors[2];
		dcolors[3] = colors[3] * ps->darken;
		memcpy (dcolors + 4, dcolors, colorSize);
		memcpy (dcolors + 8, dcolors, colorSize);
		memcpy (dcolors + 12, dcolors, colorSize);

		dcolors += 16;
	    }
	}
    }

    glEnableClientState (GL_COLOR_ARRAY);

    glTexCoordPointer (2, GL_FLOAT, 2 * sizeof (GLfloat), ps->coords_cache);
    glVertexPointer (3, GL_FLOAT, 3 * sizeof (GLfloat), ps->vertices_cache);

    // darken the background
    if (ps->darken > 0)
    {
	glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	glColorPointer (4, GL_FLOAT, 4 * sizeof (GLfloat), ps->dcolors_cache);
	glDrawArrays (GL_QUADS, 0, numActive);
    }
    // draw particles
    glBlendFunc (GL_SRC_ALPHA, ps->blendMode);

    glColorPointer (4, GL_FLOAT, 4 * sizeof (GLfloat), ps->colors_cache);

    glDrawArrays (GL_QUADS, 0, numActive);

    glDisableClientState (GL_COLOR_ARRAY);

    glPopMatrix ();
    glColor4usv (defaultColor);
    screenTexEnvMode (s, GL_REPLACE);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_TEXTURE_2D);
    glDisable (GL_BLEND);
}

static void
updateParticles (ParticleSystem * ps, float time)
{
    int i, j;
    int newCount = 0;
    Particle *part;
    GPoint *gi;
    float gdist, gangle;
    ps->active = FALSE;

    part = ps->particles;
    for (i = 0; i < ps->hardLimit; i++, part++)
    {
	if (part->t > 0.0f)
	{
	    // move particle
	    part->x += part->vx * time;
	    part->y += part->vy * time;

	    // Rotation
	    part->phi += part->vphi*time;

	    //Aging of particles
	    part->t += part->vt * time;
	    //Additional aging of particles increases if softLimit is exceeded
	    if (ps->lastCount > ps->softLimit)
		part->t += part->vt * time * (ps->lastCount - ps->softLimit)
					/ (ps->hardLimit - ps->softLimit);

	    //Global gravity
	    part->vx += ps->gx * time;
	    part->vy += ps->gy * time;

	    //GPoint gravity
	    gi = ps->g;
	    for (j = 0; j < ps->ng; j++, gi++)
	    {
		if (gi->strength != 0)
		{
		    gdist = sqrt ((part->x-gi->x)*(part->x-gi->x)
				    + (part->y-gi->y)*(part->y-gi->y));
		    if (gdist > 1)
		    {
			gangle = atan2 (gi->y-part->y, gi->x-part->x);
			part->vx += gi->strength / gdist * cos (gangle) * time;
			part->vy += gi->strength / gdist * sin (gangle) * time;
		    }
		}
	    }

	    ps->active  = TRUE;
	    newCount++;
	}
    }
    ps->lastCount = newCount;

    //Particle gravity
    Particle *gpart;
    part = ps->particles;
    for (i = 0; i < ps->hardLimit; i++, part++)
    {
	if (part->t > 0.0f && part->g != 0)
	{
	    gpart = ps->particles;
	    for (j = 0; j < ps->hardLimit; j++, gpart++)
	    {
		if (gpart->t > 0.0f)
		{
		    gdist = sqrt ((part->x-gpart->x)*(part->x-gpart->x)
				    + (part->y-gpart->y)*(part->y-gpart->y));
		    if (gdist > 1)
		    {
			gangle = atan2 (part->y-gpart->y, part->x-gpart->x);
			gpart->vx += part->g/gdist* cos (gangle) * part->t*time;
			gpart->vy += part->g/gdist* sin (gangle) * part->t*time;
		    }
		}
	    }
	}
    }
}

static void
finiParticles (ParticleSystem * ps)
{
    free (ps->e);
    free (ps->particles);
    if (ps->tex)
	glDeleteTextures (1, &ps->tex);

    if (ps->vertices_cache)
	free (ps->vertices_cache);
    if (ps->colors_cache)
	free (ps->colors_cache);
    if (ps->coords_cache)
	free (ps->coords_cache);
    if (ps->dcolors_cache)
	free (ps->dcolors_cache);
}

static void
genNewParticles (ParticleSystem *ps, Emitter *e)
{

    float q, p, t, h, l;
    int count = e->count;

    Particle *part = ps->particles;
    int i, j;

    t = 0.0f;

    for (i = 0; i < ps->hardLimit && count > 0; i++, part++)
    {
	if (part->t <= 0.0f)
	{
	    //Position
	    part->x = rRange (e->x, e->dx);		// X Position
	    part->y = rRange (e->y, e->dy);		// Y Position
	    if ((q = rRange (e->dcirc/2.,e->dcirc/2.)) > 0)
	    {
		p = rRange (0, M_PI);
		part->x += q * cos (p);
		part->y += q * sin (p);
	    }

	    //Speed
	    part->vx = rRange (e->vx, e->dvx);		// X Speed
	    part->vy = rRange (e->vy, e->dvy);		// Y Speed
	    if ((q = rRange (e->dvcirc/2.,e->dvcirc/2.)) > 0)
	    {
		p = rRange (0, M_PI);
		part->vx += q * cos (p);
		part->vy += q * sin (p);
	    }
	    part->vt = rRange (e->vt, e->dvt);		// Aging speed
	    if (part->vt > -0.0001)
		part->vt = -0.0001;

	    //Size, Gravity and Rotation
	    part->s = rRange (e->s, e->ds);		// Particle size
	    part->snew = rRange (e->snew, e->dsnew);	// Particle start size
	    if (e->gp > (float)(random () & 0xffff) / 65535.)
		part->g = rRange (e->g, e->dg);		// Particle gravity
	    else
		part->g = 0.;
	    part->phi = rRange (0, M_PI);		// Random orientation
	    part->vphi = rRange (e->vphi, e->dvphi);	// Rotation speed

	    //Alpha
	    part->a = rRange (e->a, e->da);		// Alpha
	    if (part->a > 1)
		part->a = 1.;
	    else if (part->a < 0)
		part->a = 0.;

	    //HSL to RGB conversion from Wikipedia simplified by S = 1
            h = rRange (e->h, e->dh); //Random hue within range
	    if (h < 0)
		h += 1.;
            else if (t > 1)
		h -= 1.;
            l = rRange (e->l, e->dl); //Random lightness ...
	    if (l > 1)
		l = 1.;
	    else if (l < 0)
		l = 0.;
	    q = e->l * 2;
	    if (q > 1)
		q = 1.;
	    p = 2 * e->l - q;
	    for (j = 0; j < 3; j++)
	    {
		t = h + (1-j)/3.;
                if (t < 0)
		    t += 1.;
                else if (t > 1)
		    t -= 1.;
		if (t < 1/6.)
		    part->c[j] = p + ((q-p)*6*t);
		else if (t < .5)
		    part->c[j] = q;
		else if (t < 2/3.)
		    part->c[j] = p + ((q-p)*6*(2/3.-t));
		else
		    part->c[j] = p;
	    }

	    // give new life
	    part->t = 1.;

	    ps->active = TRUE;
	    count -= 1;
	}
    }
}

static void
damageRegion (CompScreen *s)
{
    REGION   r;
//    int      i;
//    Particle *p;
//    float cOff;
    float    x1, x2, y1, y2;

    WIZARD_SCREEN (s);

    if (!ws->ps)
	return;

    x1 = 0;
    x2 = s->width;
    y1 = 0;
    y2 = s->height;

/*  // I don't know much about this system, but since this plugin is meant to be
    // a heavy toy, which occupies almost the whole screen, calculating the
    // exact region each time seems to waste more ressources than just using
    // the whole screen.
    x1 = s->width;
    x2 = 0;
    y1 = s->height;
    y2 = 0;

    p = ws->ps->particles;

    for (i = 0; i < ws->ps->hardLimit; i++, p++)
    {
	cOff = p->s / 2;
	if (p->t > ws->ps->tnew)
	    cOff += (p->snew - p->s) * (p->t - ws->ps->tnew)
		    / (1. - ws->ps->tnew) / 2.;
	else if (p->t < ws->ps->told)
	    cOff -= p->s * (ws->ps->told - p->t) / ws->ps->told / 2.;

	x1 = MIN (x1, p->x - cOff);
	x2 = MAX (x2, p->x + cOff);
	y1 = MIN (y1, p->y - cOff);
	y2 = MAX (y2, p->y + cOff);
    }
*/

    r.rects = &r.extents;
    r.numRects = r.size = 1;

    r.extents.x1 = floor (x1);
    r.extents.x2 = ceil (x2);
    r.extents.y1 = floor (y1);
    r.extents.y2 = ceil (y2);

    damageScreenRegion (s, &r);
}

static void
positionUpdate (CompScreen *s, int x, int y)
{
    WIZARD_SCREEN (s);

    ws->mx = x;
    ws->my = y;

    if (ws->ps && ws->active && ws->ps->e)
    {
	Emitter *ei = ws->ps->e;
	GPoint  *gi = ws->ps->g;
	int i;
	for (i = 0; i < ws->ps->ng; i++, gi++)
	{
	    if (gi->movement == MOVEMENT_MOUSEPOSITION)
	    {
		gi->x = x;
		gi->y = y;
	    }
	}

	for (i = 0; i < ws->ps->ne; i++, ei++)
	{
	    if (ei->movement == MOVEMENT_MOUSEPOSITION)
	    {
		ei->x = x;
		ei->y = y;
	    }
	    if (ei->active && ei->trigger == TRIGGER_MOUSEMOVEMENT)
		genNewParticles (ws->ps, ei);
	}
    }
}

static void
wizardPreparePaintScreen (CompScreen *s, int time)
{
    WIZARD_SCREEN (s);
    WIZARD_DISPLAY (s->display);

    if (ws->active && !ws->pollHandle)
    {
	(*wd->mpFunc->getCurrentPosition) (s, &ws->mx, &ws->my);
	ws->pollHandle = (*wd->mpFunc->addPositionPolling) (s, positionUpdate);
    }

    if (ws->active && !ws->ps)
    {
	ws->ps = calloc(1, sizeof (ParticleSystem));
	if (!ws->ps)
	{
	    UNWRAP (ws, s, preparePaintScreen);
	    (*s->preparePaintScreen) (s, time);
	    WRAP (ws, s, preparePaintScreen, wizardPreparePaintScreen);
	    return;
	}
	loadGPoints (s, ws->ps);
	loadEmitters (s, ws->ps);
	initParticles (wizardGetHardLimit (s), wizardGetSoftLimit (s), ws->ps);
	ws->ps->darken = wizardGetDarken (s);
	ws->ps->blendMode = (wizardGetBlend (s)) ? GL_ONE :
				GL_ONE_MINUS_SRC_ALPHA;
	ws->ps->tnew = wizardGetTnew (s);
	ws->ps->told = wizardGetTold (s);
	ws->ps->gx = wizardGetGx (s);
	ws->ps->gy = wizardGetGy (s);

	glGenTextures (1, &ws->ps->tex);
	glBindTexture (GL_TEXTURE_2D, ws->ps->tex);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, particleTex);
	glBindTexture (GL_TEXTURE_2D, 0);
    }

    if (ws->ps && ws->active && ws->ps->e)
    {
	Emitter *ei = ws->ps->e;
	GPoint *gi = ws->ps->g;
	int i;

	for (i = 0; i < ws->ps->ng; i++, gi++)
	{
	    if (gi->movement==MOVEMENT_BOUNCE || gi->movement==MOVEMENT_WRAP)
	    {
		gi->x += gi->espeed * cos (gi->eangle) * time;
		gi->y += gi->espeed * sin (gi->eangle) * time;
		if (gi->x >= s->width)
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->x = 2*s->width - gi->x - 1;
			gi->eangle = M_PI - gi->eangle;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->x -= s->width;
		}
		else if (gi->x < 0)
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->x *= -1;
			gi->eangle = M_PI - gi->eangle;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->x += s->width;
		}
		if (gi->y >= s->height)
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->y = 2*s->height - gi->y - 1;
			gi->eangle *= -1;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->y -= s->height;
		}
		else if (gi->y < 0)
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->y *= -1;
			gi->eangle *= -1;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->y += s->height;
		}
	    }
	    if (gi->movement==MOVEMENT_FOLLOWMOUSE
		&& (ws->my!=gi->y||ws->mx!=gi->x))
	    {
		gi->eangle = atan2(ws->my-gi->y, ws->mx-gi->x);
		gi->x += gi->espeed * cos(gi->eangle) * time;
		gi->y += gi->espeed * sin(gi->eangle) * time;
	    }
	}

	for (i = 0; i < ws->ps->ne; i++, ei++)
	{
	    if (ei->movement==MOVEMENT_BOUNCE || ei->movement==MOVEMENT_WRAP)
	    {
		ei->x += ei->espeed * cos (ei->eangle) * time;
		ei->y += ei->espeed * sin (ei->eangle) * time;
		if (ei->x >= s->width)
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->x = 2*s->width - ei->x - 1;
			ei->eangle = M_PI - ei->eangle;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->x -= s->width;
		}
		else if (ei->x < 0)
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->x *= -1;
			ei->eangle = M_PI - ei->eangle;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->x += s->width;
		}
		if (ei->y >= s->height)
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->y = 2*s->height - ei->y - 1;
			ei->eangle *= -1;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->y -= s->height;
		}
		else if (ei->y < 0)
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->y *= -1;
			ei->eangle *= -1;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->y += s->height;
		}
	    }
	    if (ei->movement==MOVEMENT_FOLLOWMOUSE
		&& (ws->my!=ei->y||ws->mx!=ei->x))
	    {
		ei->eangle = atan2 (ws->my-ei->y, ws->mx-ei->x);
		ei->x += ei->espeed * cos (ei->eangle) * time;
		ei->y += ei->espeed * sin (ei->eangle) * time;
	    }
	    if (ei->trigger == TRIGGER_RANDOMPERIOD
		&& ei->set_active && !((int)random ()&0xff))
		ei->active = !ei->active;
	    if (ei->active && (
		(ei->trigger == TRIGGER_PERSISTENT) ||
		(ei->trigger == TRIGGER_RANDOMSHOT && !((int)random()&0xff)) ||
		(ei->trigger == TRIGGER_RANDOMPERIOD)
		))
		genNewParticles (ws->ps, ei);
	}
    }

    if (ws->ps && ws->ps->active)
    {
	updateParticles (ws->ps, time);
	damageRegion (s);
    }

    UNWRAP (ws, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, time);
    WRAP (ws, s, preparePaintScreen, wizardPreparePaintScreen);
}

static void
wizardDonePaintScreen (CompScreen *s)
{
    WIZARD_SCREEN (s);
    WIZARD_DISPLAY (s->display);

    if (ws->active || (ws->ps && ws->ps->active))
	damageRegion (s);

    if (!ws->active && ws->pollHandle)
    {
	(*wd->mpFunc->removePositionPolling) (s, ws->pollHandle);
	ws->pollHandle = 0;
    }

    if (!ws->active && ws->ps && !ws->ps->active)
    {
	finiParticles (ws->ps);
	free (ws->ps);
	ws->ps = NULL;
    }

    UNWRAP (ws, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ws, s, donePaintScreen, wizardDonePaintScreen);
}

static Bool
wizardPaintOutput (CompScreen           *s,
		const ScreenPaintAttrib *sa,
		const CompTransform     *transform,
		Region                  region,
		CompOutput              *output,
		unsigned int            mask)
{
    Bool           status;
    CompTransform  sTransform;

    WIZARD_SCREEN (s);

    UNWRAP (ws, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ws, s, paintOutput, wizardPaintOutput);

    if (!ws->ps || !ws->ps->active)
	return status;

    matrixGetIdentity (&sTransform);

    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

    glPushMatrix ();
    glLoadMatrixf (sTransform.m);

    drawParticles (s, ws->ps);

    glPopMatrix ();

    glColor4usv (defaultColor);

    return status;
}

static Bool
wizardTerminate (CompDisplay    *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	WIZARD_SCREEN (s);

	ws->active = FALSE;
	damageRegion (s);

	return TRUE;
    }
    return FALSE;
}

static Bool
wizardInitiate (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid    = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	WIZARD_SCREEN (s);

	if (ws->active)
	    return wizardTerminate (d, action, state, option, nOption);

	ws->active = TRUE;

	return TRUE;
    }
    return FALSE;
}

static Bool
wizardInitScreen (CompPlugin *p, CompScreen *s)
{
    WIZARD_DISPLAY (s->display);

    WizardScreen *ws = (WizardScreen *) calloc (1, sizeof (WizardScreen) );

    if (!ws)
	return FALSE;

    s->base.privates[wd->screenPrivateIndex].ptr = ws;

    WRAP (ws, s, paintOutput, wizardPaintOutput);
    WRAP (ws, s, preparePaintScreen, wizardPreparePaintScreen);
    WRAP (ws, s, donePaintScreen, wizardDonePaintScreen);

    ws->active = FALSE;

    ws->pollHandle = 0;

    ws->ps  = NULL;

    return TRUE;
}

static void
wizardFiniScreen (CompPlugin *p, CompScreen *s)
{
    WIZARD_SCREEN (s);
    WIZARD_DISPLAY (s->display);

    //Restore the original function
    UNWRAP (ws, s, paintOutput);
    UNWRAP (ws, s, preparePaintScreen);
    UNWRAP (ws, s, donePaintScreen);

    if (ws->pollHandle)
	(*wd->mpFunc->removePositionPolling) (s, ws->pollHandle);

    if (ws->ps && ws->ps->active)
	damageScreen (s);

    //Free the pointer
    free (ws);
}

static void
wizardDisplayOptionChanged (CompDisplay        *d,
		CompOption         *opt,
		WizardDisplayOptions num)
{
    switch (num)
    {
    case WizardDisplayOptionDefaultEnabled:
	{
	    CompScreen *s;
	    for (s = d->screens; s; s = s->next)
	    {
		WIZARD_SCREEN (s);
		ws->active = wizardGetDefaultEnabled(s->display);
		damageScreen (s);
	    }
	}
	break;
    default:
	break;
    }
}

static Bool
wizardInitDisplay (CompPlugin *p, CompDisplay *d)
{
    //Generate a wizard display
    WizardDisplay *wd;
    int              index;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
        !checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "mousepoll", &index))
	return FALSE;

    wd = (WizardDisplay *) malloc (sizeof (WizardDisplay));

    if (!wd)
	return FALSE;

    //Allocate a private index
    wd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    //Check if its valid
    if (wd->screenPrivateIndex < 0)
    {
	//Its invalid so free memory and return
	free (wd);
	return FALSE;
    }

    wd->mpFunc = d->base.privates[index].ptr;

    wizardSetInitiateInitiate (d, wizardInitiate);
    wizardSetInitiateTerminate (d, wizardTerminate);
    wizardSetDefaultEnabledNotify (d, wizardDisplayOptionChanged);

    //Record the display
    d->base.privates[displayPrivateIndex].ptr = wd;
    return TRUE;
}

static void
wizardFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    WIZARD_DISPLAY (d);
    //Free the private index
    freeScreenPrivateIndex (d, wd->screenPrivateIndex);
    //Free the pointer
    free (wd);
}



static Bool
wizardInit (CompPlugin * p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
wizardFini (CompPlugin * p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
wizardInitObject (CompPlugin *p, CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) wizardInitDisplay,
	(InitPluginObjectProc) wizardInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
wizardFiniObject (CompPlugin *p, CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) wizardFiniDisplay,
	(FiniPluginObjectProc) wizardFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable wizardVTable = {
    "wizard",
    0,
    wizardInit,
    wizardFini,
    wizardInitObject,
    wizardFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &wizardVTable;
}
