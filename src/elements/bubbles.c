#include "elements-internal.h"

/* BUBBLES SPECIFIC FUNCTIONS */

void
initiateBubbleElement (CompScreen *s,
		       Element    *e)
{
    int           i;
    float         temp, xSway;
    BubbleElement *be;

    if (!e->ptr)
	e->ptr = calloc (1, sizeof (BubbleElement));

    if (!e->ptr)
	return;

    be = (BubbleElement *) e->ptr;

    temp  = elementsMmRand (elementsGetViscosity (s) / 2.0,
			    elementsGetViscosity (s), 50.0);
    xSway = 1.0 - temp * temp / 4.0;

    for (i = 0; i < MAX_AUTUMN_AGE; i++)
	be->bubbleFloat[0][i] = -xSway +
				(i * ((2 * xSway) / (MAX_AUTUMN_AGE - 1)));

    be->bubbleAge[0] = elementsGetRand (0, MAX_AUTUMN_AGE - 1);
    be->bubbleAge[1] = be->bubbleAge[0];
    be->bubbleChange = 1;

    e->x  = elementsMmRand (0, s->width, 1);
    e->y  = elementsMmRand (s->height + 100, s->height, 1);
    e->dy = elementsMmRand (-2, -1, 5);
}

void
bubbleMove (CompScreen       *s,
	    ElementAnimation *anim,
	    Element          *e,
	    int              updateDelay)
{
    float         bubblesSpeed = anim->speed / 30.0f;
    BubbleElement *be = (BubbleElement *) e->ptr;

    if (!be)
	return;

    e->x += (be->bubbleFloat[0][be->bubbleAge[0]] * (float) updateDelay) / 8;
    e->y += (e->dy * (float) updateDelay) * bubblesSpeed;
    e->z += (e->dz * (float) updateDelay) * bubblesSpeed / 100.0;
    e->rAngle += ((float) updateDelay) / (10.1f - e->rSpeed);

    be->bubbleAge[0] += be->bubbleChange;
    if (be->bubbleAge[0] >= MAX_AUTUMN_AGE)
    {
	be->bubbleAge[0] = MAX_AUTUMN_AGE - 1;
	be->bubbleChange = -9;
    }
    if (be->bubbleAge[0] <= -1)
    {
	be->bubbleAge[0] = 0;
	be->bubbleChange = 9;
    }
}

void
bubbleFini (CompScreen *s,
	    Element    *e)
{
    BubbleElement *be = (BubbleElement *) e->ptr;

    if (be)
	free (be);

    e->ptr = NULL;
}
