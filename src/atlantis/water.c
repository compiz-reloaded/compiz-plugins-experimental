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
 * Original water.c code by Dennis Kasprzyk.
 */

/*
 * Water amplitude calculation, ripple effect, deformation
 * code, and some other modifications by David Mikos.
 */


#include "atlantis-internal.h"
#include "math.h"
#include "atlantis_options.h"

static void
genTriMesh (Vertex       *vertices,
            int		 size,
	    unsigned int *indices,
	    int		 subdiv,
	    float	 r,
	    Vertex	 a)
{
    int          nRow;
    Vertex       *v;
    unsigned int *idx;
    int          i, j, k;

    float ang, d, dy, dx, y, x;

    float angStep = 2 * PI / size;

    int c  = 1; /* counter for how many points already indexed */
    int c2 = 0; /* similar to c but for indices add one for each layer */
    int oc = 1;
    int t;

    if (subdiv < 0)
	return;
    if (!vertices || !indices)
	return;

    nRow = (subdiv) ? (2 << (subdiv - 1)) : 1;

    v =   vertices;
    idx = indices;

    v[0] = a;


    /* initialize coordinates, spiralling around from centre */
    for (i = 1; i <= nRow; i++)
    {
	ang = PI / size;
	d   = i * r/nRow;

	for (j = 0; j < size; j++)
	{
	    x = d * cosf (ang);
	    y = d * sinf (ang);

	    ang -= angStep;

	    dx = (d * cosf (ang) - x) / i;
	    dy = (d * sinf (ang) - y) / i;

	    c2 = i * j + c;
	    for (k = 0; k < i; k++, c2++)
	    {
		v[c2].v[0] = y + k * dy + a.v[0];
		v[c2].v[1] = a.v[1];
		v[c2].v[2] = x + k*dx + a.v[2];
	    }
	}

	c += i * size;
    }

    c = 1;
    c2 = 0;

    int out = 1; /* outer */
    int inn = 0; /* inner */

    for (i = 1; i <= nRow; i++)
    {
	for (j = 0; j < size; j++)
	{
	    t = (2 * i - 1) * j + c2;
	    for (k = 0; k < 2 * i - 1; k++)
	    {
		idx[3 * (t + k) + 0] = out;
		idx[3 * (t + k) + 1] = inn;

		if (k % 2 == 0)
		{
		    out++;
		    if (out >= c + i * size)
			out -= i * size;

		    idx[3 * (t + k) + 2] = out;
		}
		else
		{
		    inn++;
		    if (inn >= c)
			inn -= (i - 1) * size;

		    idx[3 * (t + k) + 2] = inn;
		}
	    }
	}

	oc = c;

	c += i * size;
	c2 += (2 * i - 1) * size;

	inn = oc;
	out = c;
    }

}

static void
genTriWall (Vertex       *lVer,
	    Vertex       *hVer,
	    unsigned int *indices,
	    unsigned int idxBaseL,
	    unsigned int idxBaseH,
	    int          subdiv,
	    float        ang,
	    Vertex       a,
	    Vertex       b,
	    Vertex       c,
	    Vertex       d)
{
    int   nRow;
    int   i, k;
    float vab[3], vcd[3];

    if (subdiv < 0)
	return;
    if (!lVer || !hVer || !indices)
	return;

    nRow = pow (2, subdiv);

    for (i = 0; i < 3; i++)
    {
	vab[i] = b.v[i] - a.v[i];
	vab[i] /= nRow;
	vcd[i] = d.v[i] - c.v[i];
	vcd[i] /= nRow;
    }

    for (i = 0; i <= nRow; i++)
    {
	for (k = 0; k < 3; k++)
	{
	    lVer[i].v[k] = a.v[k] + (i * vab[k]);
	    hVer[i].v[k] = c.v[k] + (i * vcd[k]);
	}

	lVer[i].n[0] = sinf (ang);
	lVer[i].n[1] = 0;
	lVer[i].n[2] = cosf (ang);

	hVer[i].n[0] = lVer[i].n[0];
	hVer[i].n[1] = lVer[i].n[1];
	hVer[i].n[2] = lVer[i].n[2];
    }

    for (i = 0; i < nRow; i++)
    {
	indices[(i * 6)]     = idxBaseL + i;
	indices[(i * 6) + 1] = idxBaseH + i;
	indices[(i * 6) + 2] = idxBaseH + i + 1;
	indices[(i * 6) + 3] = idxBaseL + i + 1;
	indices[(i * 6) + 4] = idxBaseL + i;
	indices[(i * 6) + 5] = idxBaseH + i + 1;
    }
}

static void
updateRipple (Water *w,
              int size)
{
    int i;

    if (!w->rippleFactor)
	return;

    for (i = 0; i < w->nSVer; i++)
	w->rippleFactor[i] = NRAND (1001) - 500;
}

static Water *
genWater (int size,
          int sDiv,
          float distance,
          float bottom,
          Bool initRipple)
{
    Water  *w;
    int    i, j;

    float  ang, r, aStep;
    int    nVer, nRow, nIdx, nWVer, nWIdx;;
    Vertex a = {{ 0.0, 0.0, 0.0 }};
    Vertex b = {{ 0.0, 0.0, 0.0 }};
    Vertex c = {{ 0.0, 0.0, 0.0 }};
    Vertex d = {{ 0.0, bottom, 0.0 }};
    Vertex e = {{ 0.0, bottom, 0.0 }};

    Vertex       *wv;
    unsigned int *wi;

    if (sDiv < 0)
	return NULL;

    if (size < 3)
	return NULL;

    w = malloc (sizeof (Water));
    if (!w)
	return NULL;

    nRow = (sDiv) ? (2 << (sDiv - 1)) : 1;
    nVer = size * ((nRow * (nRow + 1)) / 2) + 1;

    nIdx = 3 * size * (nRow * nRow);

    nWIdx = pow (2, sDiv + 1) * 3;
    nWVer = pow (2, sDiv + 1) + 2;

    w->nBIdx = nRow * size;

    w->nVertices  = nVer + (nWVer) * size;
    w->nIndices   = nIdx + (nWIdx) * size + w->nBIdx;

    w->nSVer = nVer;
    w->nSIdx = nIdx;
    w->nWVer = nWVer * size;
    w->nWIdx = nWIdx * size;

    w->size     = size;
    w->distance = distance;
    w->sDiv     = sDiv;

    w->wave1 = 0.0;
    w->wave2 = 0.0;

    w->vertices = calloc (1, sizeof (Vertex) * w->nVertices);
    if (!w->vertices)
    {
	free (w);
	return NULL;
    }

    w->indices = calloc (1, sizeof (int) * w->nIndices);
    if (!w->indices)
    {
	free (w->vertices);
	free (w);
	return NULL;
    }

    w->vertices2 = NULL;
    w->indices2  = NULL;

    r = distance / cosf (M_PI / size);
    ang = M_PI / size;
    aStep = 2 * M_PI / size;

    wv = w->vertices + nVer;
    wi = w->indices  + nIdx;

    for (i = 0; i < size; i++)
    {
	d.v[0] = b.v[0] = sinf (ang - aStep) * r;
	d.v[2] = b.v[2] = cosf (ang - aStep) * r;

	e.v[0] = c.v[0] = sinf (ang) * r;
	e.v[2] = c.v[2] = cosf (ang) * r;

	genTriWall (wv + (i * nWVer / 2), wv + ((i + size) * nWVer / 2),
		    wi + (i * nWIdx), nVer + (i * nWVer / 2),
		    nVer + ((i + size) * nWVer / 2), sDiv,
		    ang - aStep / 2, b, c, d, e);

	ang += aStep;
    }
    genTriMesh (w->vertices, size, w->indices, sDiv, r, a);


    /* bottom face indices for cylinder deformation */
    for (i = 0; i < size; i++)
    {
	wi = w->indices + w->nSIdx + w->nWIdx + i * nRow;

	for (j = 0; j < nRow; j++)
	    wi[j] = w->nSVer + ((size - 1 - i + size) * nWVer / 2) +
		    nRow - 1 - j;
    }


    w->rippleTimer = 0;

    if (initRipple)
    {
	w->rippleFactor = calloc (1, sizeof (int *) * w->nSVer);

	if (!w->rippleFactor)
	{
	    free (w->vertices);
	    free (w->indices);
	    free (w);
	    return NULL;
	}
	updateRipple(w, size);
    }
    else
	w->rippleFactor = NULL;

    return w;
}

void
freeWater (Water *w)
{
    if (!w)
	return;

    if (w->vertices)
	free (w->vertices);
    if (w->indices)
	free (w->indices);
    if (w->vertices2)
	free (w->vertices2);
    if (w->indices2)
	free (w->indices2);
    if (w->rippleFactor)
	free(w->rippleFactor);

    w->vertices     = NULL;
    w->vertices2    = NULL;
    w->indices      = NULL;
    w->indices2     = NULL;
    w->rippleFactor = NULL;
}

static void
setAmplitude (Vertex *v,
	      float  bh,
	      float  wt,
	      float  swt,
	      float  wa,
	      float  swa,
	      float  wf,
	      float  swf,
	      int ripple,
	      int ripple2)
{
    float  dx, dz, d, c;

    v->v[1] = bh + (wa  * sinf (wt  + wf  * v->v[0] * v->v[2])) +
		   (swa * sinf (swt + swf * v->v[0] * v->v[2]));

    v->v[1] = MIN (0.5, MAX (-0.5, v->v[1]));

    c =  wa  * cosf (wt  + wf  * v->v[0] * v->v[2]) * wf;
    c += swa * cosf (swt + swf * v->v[0] * v->v[2]) * swf;

    dx = c * v->v[2];
    dz = c * v->v[0];

    /* actual normal
    v->n[0] = -dx;
    v->n[1] = 1;
    v->n[2] = -dz;
    */

    v->n[0] = -0.2 * (v->v[1] - bh);
    v->n[1] = 5;
    v->n[2] = -0.2 * (v->v[1] - bh);

    if (ripple)
    {
	float sum;
	float rFactor = (ripple / 1000.0f);

	v->n[0] -= (2 * dx + 3) * rFactor + 3 * dx;
	rFactor = (ripple2 / 1000.0f);
	v->n[2] -= (2 * dz + 3) * rFactor + 3 * dz;

	if (ripple & 1)
	    rFactor = (ripple / 1000.0f);

	/* for more equal normal vector lengths */
	sum = (abs (ripple) + abs (ripple2)) / 2000.0f;

	v->n[1] *= 0.8 + 0.2 * (1 - sum) * 2 * fabsf (rFactor);
    }
    else
    {
	 v->n[0] -= (5 * dx);
	 v->n[2] -= (5 * dz);
    }

    d = sqrtf ((v->n[0] * v->n[0]) + (v->n[1] * v->n[1]) +
               (v->n[2] * v->n[2]));

    if (d == 0.0f)
	return;

    v->n[0] /= d;
    v->n[1] /= d;
    v->n[2] /= d;
}


static void
deformCylinder(CompScreen *s,
               Water  *w,
               float progress)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int          nVer, nWVer, nRow, nRowS, subdiv;
    Vertex       *v;
    int          i, j, k, l;

    float  ang, r, aStep;

    Vertex       *wv;

    int size = as->hsize;

    float ratioRadiusToSideDist;

    //Vertex a = {{ 0.0, 0.0, 0.0 }};
    Vertex b = {{ 0.0, 0.0, 0.0 }};
    Vertex c = {{ 0.0, 0.0, 0.0 }};

    float    vab[3];

    int c1 = 1; /* counter for how many points already indexed */
    int c2 = 1; /* similar to c but for indices add one for each layer */

    float dist, x, y, dx, dy;


    if (!w)
	return;
    if (w->sDiv < 0)
	return;
    if (!w->vertices)
	return;
    if (w->size != size)
	return;

    subdiv = w->sDiv;
    nRow = (subdiv)?(2 << (subdiv - 1)) : 1;
    nVer = size * ((nRow * (nRow + 1)) / 2) + 1;

    nWVer = pow (2, subdiv + 1) + 2;

    ratioRadiusToSideDist = as->radius * as->ratio / as->sideDistance;

    r = cs->distance / cosf (M_PI / size);
    ang = M_PI / size;
    aStep = 2 * M_PI / size;

    wv = w->vertices + w->nSVer;
    v =  w->vertices;

    //v[0] = a;

    /* new coordinates, spiralling around from centre */
    for (i = 1; i <= nRow; i++)
    {
	ang = PI / size;
	dist = i * r / nRow;

	for (j = 0; j < size; j++)
	{
	    x = cosf (ang);
	    y = sinf (ang);

	    ang -= aStep;
	    dx = (cosf (ang) - x) / i;
	    dy = (sinf (ang) - y) / i;

	    c2 = i * j + c1;
	    for (k = 0; k < i; k++, c2++)
	    {
		v[c2].v[0] = y + k * dy;
		v[c2].v[2] = x + k * dx;

		v[c2].v[0] += progress * (sinf (ang + aStep -
		                                (k * aStep) / i) - v[c2].v[0]);
		v[c2].v[0] *= dist;

		v[c2].v[2] += progress * (cosf (ang + aStep -
		                                (k * aStep) / i) - v[c2].v[2]);
		v[c2].v[2] *= dist;

		/* translation not needed*/
		/*
		v[c2].v[0] += a[0];
		v[c2].v[2] += a[2];
		*/
	    }
	}

	c1 += i * size;
    }

    ang = M_PI / size;

    for (l = 0; l < size; l++)
    {
	v = w->vertices + (l * nVer);

	b.v[0] = sinf (ang - aStep);
	b.v[2] = cosf (ang - aStep);

	c.v[0] = sinf (ang);
	c.v[2] = cosf (ang);

	for (i = 0; i < 3; i++)
	{
	    vab[i] = b.v[i];// - a.v[i];
	    vab[i] /= nRow - 1.0;
	}


	Vertex *lVer = wv + (l * nWVer / 2);
	Vertex *hVer = wv + ((l + size) * nWVer / 2);

	/*side walls */
	    nRowS = pow (2, subdiv);

	    for (i = 0; i < 3; i++)
	    {
		vab[i] = c.v[i] - b.v[i];
		vab[i] /= nRowS;
	    }

	    for (i = 0; i <= nRowS; i++)
	    {
		for (k = 0; k < 3; k += 2)
		{
		    lVer[i].v[k] = b.v[k] + (i * vab[k]);
		}

		float th = atan2f(lVer[i].v[0], lVer[i].v[2]);


		lVer[i].v[0] += progress * (sinf (ang - aStep + i * aStep /
		                                  nRowS) - lVer[i].v[0]);
		lVer[i].v[2] += progress * (cosf (ang - aStep + i * aStep /
		                                  nRowS) - lVer[i].v[2]);
		lVer[i].v[0] *= r;
		lVer[i].v[2] *= r;

		for (k = 0; k < 3; k += 2)
		    hVer[i].v[k] = lVer[i].v[k];


		lVer[i].n[0] = (1 - progress) * sinf (ang) +
			       progress * sinf (th);
		lVer[i].n[1] = 0;
		lVer[i].n[2] = (1-progress)*cosf(ang) +
			       progress*cosf(th);

		hVer[i].n[0] = lVer[i].n[0];
		hVer[i].n[1] = lVer[i].n[1];
		hVer[i].n[2] = lVer[i].n[2];
	    }

	ang += aStep;
    }
}

static void
deformSphere(CompScreen *s,
             Water  *w,
             float progress,
             float waterBottom,
             Bool groundNormals)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int          nWVer, nWIdx, nWVer2, nWIdx2, nRow, nRowS, subdiv;
    Vertex       *v;
    int          i, j, k, l;

    float        ang, r, aStep;

    Vertex       *wv;

    int size = as->hsize;

    float ratioRadiusToSideDist, sphereRadiusFactor, sphereRadiusFactor2;

    //Vertex a = {{ 0.0, 0.0, 0.0 }};
    Vertex b = {{ 0.0, 0.0, 0.0 }};
    Vertex c = {{ 0.0, 0.0, 0.0 }};

    float    vab[3];

    int c1 = 1; /* counter for how many points already indexed */
    int c2 = 1; /* similar to c but for indices add one for each layer */

    float dist, factor, x, y, dx, dy;

    if (!w)
	return;
    if (w->sDiv < 0)
	return;
    if (!w->vertices)
	return;
    if (w->size != size)
	return;

    subdiv = w->sDiv;
    nRow = (subdiv)?(2 << (subdiv - 1)) : 1;

    nWIdx = pow (2, subdiv + 1) * 3;
    nWVer = pow (2, subdiv + 1) + 2;

    nWIdx2 = nWIdx * nRow * 2;
    nWVer2 = nWVer * (nRow + 1) / 2;

    ratioRadiusToSideDist = as->radius * as->ratio / as->sideDistance;

    sphereRadiusFactor  = as->radius / 100000;
    sphereRadiusFactor  = progress * (hypotf (sphereRadiusFactor, 0.5f) /
			  sphereRadiusFactor - 1);
    sphereRadiusFactor2 = sphereRadiusFactor * cosf (w->bh*PI)+1;

    r = cs->distance / cosf (M_PI / size);
    ang = M_PI / size;
    aStep = 2 * M_PI / size;

    wv = w->vertices + w->nSVer;

    if (nWVer2 * size != w->nWVer2 && w->vertices2)
    {
	free (w->vertices2);
	w->vertices2 = NULL;
    }
    if (nWIdx2 * size != w->nWIdx2 && w->indices2)
    {
	free (w->indices2);
	w->indices2 = NULL;
    }

    w->nWVer2 = nWVer2 * size;
    w->nWIdx2 = nWIdx2 * size;

    w->nBIdx2 = nRow * size;

    if (!w->vertices2)
    {
	w->vertices2 = calloc (1, sizeof (Vertex) * w->nWVer2);
	if (!w->vertices2)
	    return;
    }

    if (!w->indices2)
    {
	w->indices2 = calloc (1, sizeof (int) * (w->nWIdx2 + w->nBIdx2));
    	if (!w->indices2)
    	    return;
    }

    v = w->vertices;

    //v[0] = a;

    /* new coordinates, spiralling around from centre */
    for (i = 1; i <= nRow; i++)
    {
	ang = PI / size;
	dist = i * r / nRow;
	factor = dist * sphereRadiusFactor2;

	for (j = 0; j < size; j++)
	{
	    x = cosf (ang);
	    y = sinf (ang);

	    ang -= aStep;
	    dx = (cosf (ang) - x) / i;
	    dy = (sinf (ang) - y) / i;

	    c2 = i * j + c1;
	    for (k = 0; k < i; k++, c2++)
	    {
		v[c2].v[0] = y + k * dy;
		v[c2].v[2] = x + k * dx;

		v[c2].v[0] += progress * (sinf (ang + aStep -
		                                (k * aStep) / i) - v[c2].v[0]);
		v[c2].v[2] += progress * (cosf (ang + aStep -
		                                (k * aStep) / i) - v[c2].v[2]);
		v[c2].v[0] *= factor;
		v[c2].v[2] *= factor;

		/* translation not needed*/
		/*
		v[c2].v[0] += a[0];
		v[c2].v[2] += a[2];
		*/
	    }
	}

	c1 += i * size;
    }

    ang = M_PI / size;

    for (l = 0; l < size; l++)
    {
	unsigned int * indices = w->indices2 + (l * nWIdx);
	unsigned int idxBaseL = (l * nWVer / 2);

	b.v[0] = sinf (ang - aStep);
	b.v[2] = cosf (ang - aStep);

	c.v[0] = sinf (ang);
	c.v[2] = cosf (ang);

	Vertex *lVer = w->vertices2 + (l * nWVer2 / (nRow + 1));

	/*side walls */
	nRowS = pow (2, subdiv);

	for (i = 0; i < 3; i++)
	{
	    vab[i] = c.v[i] - b.v[i];
	    vab[i] /= nRowS;
	}

	for (i = 0; i <= nRowS; i++)
	{
	    float th;

	    for (k = 0; k < 3; k++)
		lVer[i].v[k] = b.v[k] + (i * vab[k]);

	    lVer[i].v[0] += progress * (sinf (ang - aStep + i * aStep /
	                                      nRowS) - lVer[i].v[0]);
	    lVer[i].v[2] += progress * (cosf (ang - aStep + i * aStep /
	                                      nRowS) - lVer[i].v[2]);

	    th = atan2f (lVer[i].v[0], lVer[i].v[2]);

	    lVer[i].n[0] = (1 - progress) * sinf (ang - aStep / 2) +
			   progress * sinf (th);
	    lVer[i].n[1] = 0;
	    lVer[i].n[2] = (1 - progress) * cosf (ang - aStep / 2) +
			   progress * cosf (th);

	    for (j = nRow; j >= 0; j--)
	    {
		Vertex *hVer = lVer + j * (size * nWVer2 / (nRow + 1));

		float hFactor;
		float p = ((float) j) / nRow;

		for (k = 0; k < 3; k++)
		{
		    hVer[i].v[k] = lVer[i].v[k];
		    hVer[i].n[k] = lVer[i].n[k];
		}

		hVer[i].n[0] = p * ((1 - progress) * sinf (ang - aStep / 2) +
			       progress * sinf (th));
		hVer[i].n[1] = 1 - p;
		hVer[i].n[2] = p * ((1 - progress) * cosf (ang - aStep / 2) +
			       progress * cosf(th));

		hFactor = r * (sphereRadiusFactor * cosf ((w->bh - j *
			  (w->bh - waterBottom) / nRow) * PI)+1);

		for (k = 0; k < 3; k += 2)
		    hVer[i].v[k] *= hFactor;
	    }
	}

	for (j = 0; j < nRow; j++)
	{
	    unsigned int idxBaseH = idxBaseL + size * nWVer / 2;

	    for (i = 0; i < nRowS; i++)
	    {
		indices[(i * 6)]     = idxBaseL + i;
		indices[(i * 6) + 1] = idxBaseH + i;
		indices[(i * 6) + 2] = idxBaseH + i + 1;
		indices[(i * 6) + 3] = idxBaseL + i + 1;
		indices[(i * 6) + 4] = idxBaseL + i;
		indices[(i * 6) + 5] = idxBaseH + i + 1;
	    }
	    idxBaseL = idxBaseH;
	    indices += 2 * nWIdx2 / nRow;
	}

	/* bottom face indices */
	idxBaseL = (nRow - 1) * size * nWVer / 2;
	indices = w->indices2 + w->nWIdx2 + (l * nRow);

	for (j = 0; j < nRow; j++)
	    indices[j] = idxBaseL + ((size - 1 - l + size) * nWVer / 2) +
			 nRow - 1 - j;

	ang += aStep;
    }
}

void
updateHeight (Water *w,
              Water *w2,
              Bool rippleEffect,
              int currentDeformation)
{
    int offset;

    Bool useOtherWallVertices;
    Vertex * vertices;

    int i, j;

    if (!w)
	return;

    offset = w->nSVer / 2 + 1;
    rippleEffect = (rippleEffect && w->rippleFactor);

    useOtherWallVertices = (currentDeformation == DeformationSphere &&
	    		    w->vertices2);
    vertices = (useOtherWallVertices ? w->vertices2 - w->nSVer : w->vertices);

    for (i = 0; i < w->nSVer; i++)
	setAmplitude(&w->vertices[i], w->bh, w->wave1, w->wave2, w->wa,
	             w->swa, w->wf, w->swf,
	             (rippleEffect ? w->rippleFactor[i] : 0),
	             (rippleEffect ? w->rippleFactor[(i + offset) % w->nSVer] :
	             		     0));

    for (i = w->nSVer; i < w->nSVer + (w->nWVer / 2); i++)
        setAmplitude(&vertices[i], w->bh, w->wave1, w->wave2, w->wa,
		     w->swa, w->wf, w->swf, 0, 0);

    if (useOtherWallVertices)
    {
	int nRow = (w->sDiv)?(2 << (w->sDiv - 1)) + 1 : 2;

	Vertex * verticesL = vertices;

	for (j = 1; j < nRow - 1; j++ )
	{
	    vertices += w->nWVer / 2;

	    for (i=w->nSVer; i < w->nSVer + (w->nWVer / 2); i++)
		vertices[i].v[1] = verticesL[i].v[1] - j *
				   (verticesL[i].v[1] + 0.5) / (nRow - 1);
	}

	vertices += w->nWVer / 2;

	 /* set bottom ground to base of deformed cube */
	 /* this is okay because ground and water have same grid size */
	    for (i = w->nSVer; i < w->nSVer + (w->nWVer / 2); i++)
	        vertices[i].v[1] = -0.5;
    }
}

void
updateDeformation (CompScreen *s,
                   int currentDeformation)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    static const float floatErr = 0.0001f;

    Bool deform = FALSE;

    float progress, dummy;
    (*cs->getRotation) (s, &dummy, &dummy, &progress);

    if (currentDeformation == DeformationNone)
    {
	if (as->oldProgress == 0.0f)
	    return;

	as->oldProgress = 0.0f;
	progress = 0.0f;
    }
    else
    {
	if (fabsf (progress) < floatErr)
	    progress = 0.0f;
	else if (fabsf (1.0f - progress) < floatErr)
	    progress = 1.0f;

	if ((as->oldProgress != 0.0f || progress != 0.0f) &&
		(as->oldProgress != 1.0f || progress != 1.0f))
	{
	    if (progress == 0.0f || progress == 1.0f)
	    {
		if (as->oldProgress != progress)
		{
		    deform = TRUE;
		    as->oldProgress = progress;
		}
	    }
	    else if (fabsf (as->oldProgress - progress) >= floatErr)
	    {
		deform = TRUE;
		as->oldProgress = progress;
	    }
	}
    }

    if (deform)
    {
	if (atlantisGetShowWater (s) || atlantisGetShowWaterWire (s))
	{
	    switch (currentDeformation)
	    {
	    case DeformationNone :
	    case DeformationCylinder :
		deformCylinder(s, as->water, progress);
		break;

	    case DeformationSphere :
		deformSphere(s, as->water, progress, -0.5, FALSE);
	    }
	}

	if (atlantisGetShowGround (s))
	{
	    switch (currentDeformation)
	    {
	    case DeformationNone :
		progress = 0.0f;
	    case DeformationCylinder :
		deformCylinder (s, as->ground, progress);
		break;

	    case DeformationSphere :
		deformSphere (s, as->ground, progress, -0.5, TRUE);
	    }

	    updateHeight (as->ground, NULL, FALSE, currentDeformation);
	}
    }
}

void
updateWater (CompScreen *s,
             float time)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int sDiv = (atlantisGetRenderWaves (s))?
	       atlantisGetGridQuality (s) : 0;
    int size = as->hsize;

    if (!as->water)
	as->water = genWater (size, sDiv, cs->distance, -0.5, atlantisGetWaveRipple (s));

    if (!as->water)
	return;

    if (as->water->size != size || as->water->sDiv != sDiv ||
	as->water->distance != cs->distance || (atlantisGetWaveRipple (s) && !as->water->rippleFactor))
    {
	freeWater (as->water);
	as->water = genWater (size, sDiv, cs->distance, -0.5, atlantisGetWaveRipple (s));

	if (!as->water)
	    return;
    }

    if (atlantisGetWaveRipple (s))
    {
	as->water->rippleTimer -= (int) (time * 1000);
	if (as->water->rippleTimer <= 0)
	{
	    as->water->rippleTimer += 170;
	    updateRipple (as->water, size);
	}
    }

    as->water->wave1 += time * as->speedFactor;
    as->water->wave2 += time * as->speedFactor;

    as->water->wave1 = fmodf (as->water->wave1, 2 * M_PI);
    as->water->wave2 = fmodf (as->water->wave2, 2 * M_PI);

    if (atlantisGetRenderWaves (s))
    {
	as->water->wa  = atlantisGetWaveAmplitude (s);
	as->water->swa = atlantisGetSmallWaveAmplitude (s);
 	as->water->wf  = atlantisGetWaveFrequency (s);
	as->water->swf = atlantisGetSmallWaveFrequency (s);
    }
    else
    {
	as->water->wa  = 0.0;
	as->water->swa = 0.0;
 	as->water->wf  = 0.0;
	as->water->swf = 0.0;
    }

    as->water->bh  = -0.5 + atlantisGetWaterHeight (s);
}

void
updateGround (CompScreen *s,
              float time)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int sDiv = atlantisGetGridQuality (s);
    int size = as->hsize;

    Bool update = FALSE;

    if (!as->ground)
    {
	as->ground = genWater (size, sDiv, cs->distance, -0.5, FALSE);
	update = TRUE;
    }

    if (!as->ground)
	return;

    if (as->ground->size != size || as->ground->sDiv != sDiv ||
	as->ground->distance != cs->distance)
    {
	freeWater (as->ground);
	as->ground = genWater (size, sDiv, cs->distance, -0.5, FALSE);

	update = TRUE;
	if (!as->ground)
	    return;
    }

    if (!update)
	return;

    as->ground->wave1 = (float)(rand () & 15) / 15.0;
    as->ground->wave2 = (float)(rand () & 15) / 15.0;

    as->ground->bh  = -0.45;
    as->ground->wa  = 0.1;
    as->ground->swa = 0.02;
    as->ground->wf  = 2.0;
    as->ground->swf = 10.0;

    updateHeight (as->ground, NULL, FALSE, DeformationNone);
}

void
drawWater (Water *w, Bool full, Bool wire, int currentDeformation)
{
    float *v;
    if (!w)
	return;

    glDisable (GL_DEPTH_TEST);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (full)
    {
	glEnable  (GL_LIGHTING);
	glEnable  (GL_LIGHT1);
	glDisable (GL_LIGHT0);

	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState  (GL_NORMAL_ARRAY);

	v = (float *) w->vertices;

	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);
	glNormalPointer (GL_FLOAT,    6 * sizeof (float), v + 3);

	glDrawElements (GL_TRIANGLES, w->nSIdx, GL_UNSIGNED_INT, w->indices);

	glDisableClientState (GL_NORMAL_ARRAY);
	glDisable (GL_LIGHTING);

	glEnable (GL_COLOR_MATERIAL);
	if (currentDeformation == DeformationSphere &&
	    w->vertices2 && w->indices2)
	{
	    v = (float *) w->vertices2;

	    glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);
	    glNormalPointer (GL_FLOAT,    6 * sizeof (float), v + 3);

	    glDrawElements (GL_TRIANGLES, w->nWIdx2,
	                    GL_UNSIGNED_INT, w->indices2);
	}
	else
	    glDrawElements (GL_TRIANGLES, w->nWIdx,
	                    GL_UNSIGNED_INT, w->indices + w->nSIdx);
    }

    glDisableClientState (GL_NORMAL_ARRAY);

    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    glColor4usv (defaultColor);

    if (wire)
    {
	glDisable (GL_LIGHTING);

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	v = (float *) w->vertices;

	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);

	glDisableClientState (GL_NORMAL_ARRAY);

	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);
	glDrawElements  (GL_LINE_STRIP, w->nSIdx, GL_UNSIGNED_INT, w->indices);

	if (currentDeformation == DeformationSphere)
	{
	    v = (float *) w->vertices2;

	    glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);
	    glDrawElements  (GL_LINE_STRIP, w->nWIdx2,
	                     GL_UNSIGNED_INT, w->indices2);
	}
	else
	    glDrawElements  (GL_LINE_STRIP, w->nWIdx,
	                     GL_UNSIGNED_INT, w->indices + w->nSIdx);
    }
}

void
drawGround (Water *w,
            Water *g,
            int currentDeformation)
{
    float *v;
    float *n;

    if (!g)
	return;

    glEnable  (GL_DEPTH_TEST);

    glEnable  (GL_LIGHTING);
    glEnable  (GL_LIGHT1);
    glDisable (GL_LIGHT0);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    v = (float *) g->vertices;
    glDisableClientState (GL_TEXTURE_COORD_ARRAY);

    glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);

    glEnableClientState (GL_NORMAL_ARRAY);

    if (w)
	n = (float *) w->vertices;
    else
	n = (float *) g->vertices;
    glNormalPointer (GL_FLOAT, 6 * sizeof (float), n + 3);

    glDrawElements (GL_TRIANGLES, g->nSIdx, GL_UNSIGNED_INT, g->indices);

    if (currentDeformation == DeformationSphere && g->vertices2 && g->indices2)
    {
	v = (float *) g->vertices2;
	n = (float *) g->vertices2;

	glNormalPointer (GL_FLOAT, 6 * sizeof (float), n + 3);
	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);

	glDrawElements (GL_TRIANGLES, g->nWIdx2,
	                GL_UNSIGNED_INT, g->indices2);
    }
    else
	glDrawElements (GL_TRIANGLES, g->nWIdx,
	                GL_UNSIGNED_INT, g->indices + g->nSIdx);

    glDisableClientState (GL_NORMAL_ARRAY);
    glDisable (GL_LIGHTING);

    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
}

static void fillBottom (Water *w,
                        float distance,
                        float bottom,
                        int currentDeformation)
{
    int   i;
    float *v;
    int	  size = w->size;

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);

    if (currentDeformation == DeformationCylinder)
    {
	v = (float *) w->vertices;

	glNormal3f (0, -1, 0);

	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);
	glDisableClientState (GL_NORMAL_ARRAY);

	glDrawElements (GL_TRIANGLE_FAN, w->nBIdx,
	                GL_UNSIGNED_INT, w->indices + w->nSIdx + w->nWIdx);
    }
    else if (currentDeformation == DeformationSphere &&
	     w->vertices2 && w->indices2)
    {
	v = (float *) w->vertices2;

	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);

	glDisableClientState (GL_NORMAL_ARRAY);

	glNormal3f (0, -1, 0);

	glDrawElements (GL_TRIANGLE_FAN, w->nBIdx2,
	                GL_UNSIGNED_INT, w->indices2 + w->nWIdx2);
    }
    else
    {
	float r = distance / cosf (M_PI / size);
	float ang = M_PI / size;
	float aStep = 2 * M_PI / size;

	glBegin (GL_TRIANGLE_FAN);
	glNormal3f (0, -1, 0);
	glVertex3f (0.0, bottom, 0.0);

	for (i = 0; i <= size; i++)
	{
	    glVertex3f (sinf (ang) * r, bottom, cosf (ang) * r);
	    ang -= aStep;
	}
	glEnd ();
    }

    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
}

void
drawBottomGround (Water *w,
                  float distance,
                  float bottom,
                  int currentDeformation)
{
    glDisable (GL_DEPTH_TEST);

    glEnable  (GL_LIGHTING);
    glEnable  (GL_LIGHT1);
    glDisable (GL_LIGHT0);

    fillBottom (w, distance, bottom, currentDeformation);

    glDisable (GL_LIGHTING);
}

void
drawBottomWater (Water *w,
                 float distance,
                 float bottom,
                 int currentDeformation)
{
    glDisable (GL_DEPTH_TEST);

    glDisable (GL_LIGHTING);

    glEnable (GL_COLOR_MATERIAL);
    fillBottom (w, distance, bottom, currentDeformation);
}

void
setWaterMaterial (unsigned short * waterColor)
{
    static const float mat_shininess[]  = { 140.0 };
    static const float mat_specular[]   = { 1.0, 1.0, 1.0, 1.0 };

    glDisable (GL_COLOR_MATERIAL);
    setMaterialAmbientDiffuse4us (waterColor, 2.0, 0.8);

    glColor4usv (waterColor);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
}

void
setGroundMaterial (unsigned short * groundColor)
{
    static const float mat_shininess[]  = { 40.0 };
    static const float mat_specular[]   = { 0.0, 0.0, 0.0, 1.0 };

    glDisable (GL_COLOR_MATERIAL);
    setMaterialAmbientDiffuse4us (groundColor, 1.5, 0.8);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
}

float
getHeight (Water *w,
           float x,
           float z)
{
    if (!w)
	return 0.0;
    return w->bh + (w->wa * sinf (w->wave1 + w->wf * x * z)) +
	   (w->swa * sinf (w->wave2 + w->swf * x * z));
}

/* use other scale for creatures inside cube */
float
getGroundHeight (CompScreen *s,
                 float x,
                 float z)
{
    ATLANTIS_SCREEN (s);

    Water *g = as->ground;

    if (atlantisGetShowGround(s))
	return getHeight(g, x / (100000 * as->ratio),
	                 z / (100000 * as->ratio)) * 100000;
    return -0.5*100000;
}
