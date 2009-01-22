#include "elements-internal.h"

/* STARS SPECIFIC FUNCTIONS */

static float
starBezierCurve (float p,
		 float time)
{
    float out;

    out = p * (time + 0.01) * 10;

    return out;
}

void
initiateStarElement (CompScreen *s,
		     Element    *e)
{
    float init;

    e->dx = elementsMmRand (-50000, 50000, 5000);
    e->dy = elementsMmRand (-50000, 50000, 5000);
    e->dz = elementsMmRand (000, 200, 2000);
    e->x = s->width / 2 + elementsGetStarOffsetX (s); /* X Offset */
    e->y = s->height / 2 + elementsGetStarOffsetY (s); /* Y Offset */
    e->z = elementsMmRand (000, 0.1, 5000);
    init = elementsMmRand (0,100, 1);

    e->x += init * e->dx;
    e->y += init * e->dy;
}

void
starMove (CompScreen       *s,
	  ElementAnimation *anim,
	  Element          *e,
	  int              updateDelay)
{
    float xs, ys, zs;
    float starsSpeed = anim->speed / 500.0f;
    float tmp = 1.0f / (100.0f - starsSpeed);

    xs = starBezierCurve(e->dx, tmp);
    ys = starBezierCurve(e->dy, tmp);
    zs = starBezierCurve(e->dz, tmp);

    e->x += xs * updateDelay * starsSpeed;
    e->y += ys * updateDelay * starsSpeed;
    e->z += zs * updateDelay * starsSpeed;
}

void
starFini (CompScreen *s,
	  Element    *e)
{
}
