#include "elements-internal.h"

/* SNOW SPECIFIC FUNCTIONS */

void
initiateSnowElement (CompScreen *s,
		     Element    *e)
{
    int snowSway = elementsGetSnowSway (s);
    int boxing = elementsGetScreenBoxing (s);

    switch (elementsGetWindDirection (s))
    {
	case WindDirectionNone:
	    e->x  = elementsMmRand (-boxing, s->width + boxing, 1);
	    e->dx = elementsMmRand (-snowSway, snowSway, 1.0);
	    e->y  = elementsMmRand (-300, 0, 1);
	    e->dy = elementsMmRand (1, 3, 1);
	    break;
	case WindDirectionUp:
	    e->x  = elementsMmRand (-boxing, s->width + boxing, 1);
	    e->dx = elementsMmRand (-snowSway, snowSway, 1.0);
	    e->y  = elementsMmRand (s->height + 300, 0, 1);
	    e->dy = -elementsMmRand (1, 3, 1);
	    break;
	case WindDirectionLeft:
	    e->x  = elementsMmRand (s->width, s->width + 300, 1);
	    e->dx = -elementsMmRand (1, 3, 1);
	    e->y  = elementsMmRand (-boxing, s->height + boxing, 1);
	    e->dy = elementsMmRand (-snowSway, snowSway, 1.5);
	    break;
	case WindDirectionRight:
	    e->x  = elementsMmRand (-300, 0, 1);
	    e->dx = elementsMmRand (1, 3, 1);
	    e->y  = elementsMmRand (-boxing, s->height + boxing, 1);
	    e->dy = elementsMmRand (-snowSway, snowSway, 1.5);
	    break;
	default:
	    break;
    }
}

void
snowMove (CompScreen       *s,
	  ElementAnimation *anim,
	  Element          *e,
	  int              updateDelay)
{
    float snowSpeed = anim->speed / 500.0f;

    e->x += (e->dx * (float) updateDelay) * snowSpeed;
    e->y += (e->dy * (float) updateDelay) * snowSpeed;
    e->z += (e->dz * (float) updateDelay) * snowSpeed;
    e->rAngle += ((float) updateDelay) / (10.1f - e->rSpeed);
}

void
snowFini (CompScreen *s,
	  Element    *e)
{
}
