#include <math.h>
#include "elements-internal.h"

/* FIREFLIES SPECIFIC FUNCTIONS */

static float
glowCurve[GLOW_STAGES][4] = {
    { 0.0, 0.5, 0.5, 1.0 },
    { 1.0, 1.0, 0.5, 0.75 },
    { 0.75, 0.3, 1.2, 1.0 },
    { 1.0, 0.7, 1.5, 1.0 },
    { 1.0, 0.5, 0.5, 0.0 }
};

static float
fireflyBezierCurve (float p[4],
		    float time)
{
    float out;
    float a = p[0];
    float b = p[0] + p[1];
    float c = p[3] + p[2];
    float d = p[3];
    float e = 1.0 - time;

    out = (a * pow (e, 3)) + (3.0 * b * pow (e, 2) * time) +
	  (3.0 * c * e * pow (time, 2)) + (d * pow (time, 3));

    return out;
}

void
initiateFireflyElement (CompScreen *s,
		        Element    *e)
{
    int            i;
    FireflyElement *fe;

    if (!e->ptr)
	e->ptr = calloc (1, sizeof (FireflyElement));

    if (!e->ptr)
	return;

    fe = (FireflyElement *) e->ptr;

    e->x = elementsMmRand (0, s->width, 1);
    e->y = elementsMmRand (0, s->height, 1);

    fe->lifespan = elementsMmRand (50,1000, 100);
    fe->age      = 0.0;

    for (i = 0; i < 4; i++)
    {
	fe->dx[i] = elementsMmRand (-3000, 3000, 1000);
	fe->dy[i] = elementsMmRand (-3000, 3000, 1000);
	fe->dz[i] = elementsMmRand (-1000, 1000, 500000);
    }
}

void
fireflyMove (CompScreen       *s,
	     ElementAnimation *anim,
	     Element          *e,
	     int              updateDelay)
{
    float          ffSpeed = anim->speed / 700.0f;
    FireflyElement *fe = (FireflyElement *) e->ptr;
    int            glowStage;
    float          xs, ys, zs;

    if (!fe)
    	return;

    fe->age       += 0.01;
    fe->lifecycle = (fe->age / 10) / fe->lifespan * (ffSpeed * 70);
    glowStage     = fe->lifecycle * GLOW_STAGES;
    e->glowAlpha  = fireflyBezierCurve (glowCurve[glowStage], fe->lifecycle);

    xs = fireflyBezierCurve (fe->dx, fe->lifecycle);
    ys = fireflyBezierCurve (fe->dy, fe->lifecycle);
    zs = fireflyBezierCurve (fe->dz, fe->lifecycle);

    e->x += xs * updateDelay * ffSpeed;
    e->y += ys * updateDelay * ffSpeed;
    e->z += zs * updateDelay * ffSpeed;

}

void
fireflyFini (CompScreen *s,
	     Element    *e)
{
    if (e->ptr)
	free (e->ptr);

    e->ptr = NULL;
}
