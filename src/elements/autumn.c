#include "elements-internal.h"

/* AUTUMN FUNCTIONS */

void
initiateAutumnElement (CompScreen *s,
		       Element    *e)
{
    int           i;
    float         xSway, ySway;
    AutumnElement *ae;

    if (!e->ptr)
	e->ptr = calloc (1, sizeof (AutumnElement));

    if (!e->ptr)
	return;

    ae = (AutumnElement *) e->ptr;

    xSway = elementsMmRand (elementsGetAutumnSway (s),
			    elementsGetAutumnSway (s), 2.0);
    ySway = elementsGetAutumnYSway (s) / 20.0;

    for (i = 0; i < MAX_AUTUMN_AGE; i++)
	ae->autumnFloat[0][i] = -xSway +
				(i * ((2 * xSway) / (MAX_AUTUMN_AGE - 1)));

    for (i = 0; i < (MAX_AUTUMN_AGE / 2); i++)
	ae->autumnFloat[1][i] = -ySway +
				(i * ((2 * ySway) / (MAX_AUTUMN_AGE - 1)));

    for (; i < MAX_AUTUMN_AGE; i++)
	ae->autumnFloat[1][i] = ySway -
				(i * ((2 * ySway) / (MAX_AUTUMN_AGE - 1)));

    ae->autumnAge[0] = elementsGetRand (0, MAX_AUTUMN_AGE - 1);
    ae->autumnAge[1] = elementsGetRand (0, (MAX_AUTUMN_AGE / 2) - 1);
    ae->autumnChange = 1;

    e->x  = elementsMmRand (0, s->width, 1);    ae->autumnChange = 1;
    e->y  = -elementsMmRand (100, s->height, 1);
    e->dy = elementsMmRand (-2, -1, 5);
}

void
autumnMove (CompScreen       *s,
	    ElementAnimation *anim,
	    Element          *e,
	    int              updateDelay)
{
    float         autumnSpeed = anim->speed / 30.0f;
    AutumnElement *ae = (AutumnElement *) e->ptr;

    if (!ae)
	return;

    e->x += (ae->autumnFloat[0][ae->autumnAge[0]] * (float) updateDelay) / 80;
    e->y += (ae->autumnFloat[1][ae->autumnAge[1]] * (float) updateDelay) / 80 +
	    autumnSpeed;
    e->z += (e->dz * (float) updateDelay) * autumnSpeed / 100.0;
    e->rAngle += ((float) updateDelay) / (10.1f - e->rSpeed);

    ae->autumnAge[0] += ae->autumnChange;
    ae->autumnAge[1] += 1;

    if (ae->autumnAge[1] >= MAX_AUTUMN_AGE)
    {
	ae->autumnAge[1] = 0;
    }
    if (ae->autumnAge[0] >= MAX_AUTUMN_AGE)
    {
	ae->autumnAge[0] = MAX_AUTUMN_AGE - 1;
	ae->autumnChange = -1;
    }
    if (ae->autumnAge[0] <= -1)
    {
	ae->autumnAge[0] = 0;
	ae->autumnChange = 1;
    }
}

void
autumnFini (CompScreen *s,
	    Element    *e)
{
    if (e->ptr)
	free (e->ptr);

    e->ptr = NULL;
}
