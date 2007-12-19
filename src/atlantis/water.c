/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This is an example plugin to show how to render something inside
 * of the transparent cube
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

/* Uses water amplitude calculation by David Mikos */

#include "atlantis-internal.h"
#include "math.h"
#include "atlantis_options.h"

static void
genTriMesh (Vertex       *vertices,
	    unsigned int *indices,
	    unsigned int idxBase,
	    unsigned int subdiv,
	    Vertex       a,
	    Vertex       b,
	    Vertex       c)
{
    int          nVer, nRow;
    Vertex       *v;
    unsigned int *idx;
    int          i, j, k;
    int          tr, br;

    float    vab[3], vac[3], rb[3], re[3], ri[3];

    if (subdiv < 0)
	return;
    if (!vertices || !indices)
	return;

    nRow = (subdiv)?(2 << (subdiv - 1)) + 1 : 2;
    nVer = (nRow * (nRow + 1)) / 2;

    v =   vertices;
    idx = indices;


    for (i = 1; i < nRow; i++)
    {
	tr = (i * (i - 1)) / 2;
	br = (i * (i + 1)) / 2;

	for (j = 0; j < (2 * i) - 1; j++)
	{
	    if (j & 1)
	    {
		k = (j - 1) / 2;
		idx[(j * 3)] = idxBase + tr + k + 1;
		idx[(j * 3) + 1] = idxBase + tr + k;
		idx[(j * 3) + 2] = idxBase + br + k + 1;
	    }
	    else
	    {
		k = j / 2;
		idx[(j * 3)] = idxBase + tr + k;
		idx[(j * 3) + 1] = idxBase + br + k;
		idx[(j * 3) + 2] = idxBase + br + k + 1;
	    }
	}
	idx += ((2 * i) - 1) * 3;
    }

    for (i = 0; i < 3; i++)
    {
	vab[i] = b.v[i] - a.v[i];
	vab[i] /= nRow - 1.0;
	vac[i] = c.v[i] - a.v[i];
	vac[i] /= nRow - 1.0;
    }

    v[0] = a;

    for (i = 1; i < nRow; i++)
    {
	br = (i * (i + 1)) / 2;
	for (k = 0; k < 3; k++)
	{
	    rb[k] = a.v[k] + (i * vab[k]);
	    re[k] = a.v[k] + (i * vac[k]);
	    ri[k] = re[k] - rb[k];
	    ri[k] /= i;
	}
	for (j = 0; j <= i; j++)
	    for (k = 0; k < 3; k++)
		v[br + j].v[k] = rb[k] + (j * ri[k]);
    }

}

static void
genTriWall (Vertex       *lVer,
	    Vertex       *hVer,
	    unsigned int *indices,
	    unsigned int idxBaseL,
	    unsigned int idxBaseH,
	    int          subdiv,
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
    }

    for (i = 0; i < nRow; i++)
    {
	indices[(i * 6)] = idxBaseL + i;
	indices[(i * 6) + 1] = idxBaseH + i;
	indices[(i * 6) + 2] = idxBaseH + i + 1;
	indices[(i * 6) + 3] = idxBaseL + i + 1;
	indices[(i * 6) + 4] = idxBaseL + i;
	indices[(i * 6) + 5] = idxBaseH + i + 1;
    }
}

static Water *
genWater (int size, int sDiv, float distance, float bottom)
{

    Water  *w;
    int    i;
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

    nRow = (sDiv) ? (2 << (sDiv - 1)) + 1 : 2;
    nVer = (nRow * (nRow + 1)) / 2;
    nIdx = pow (4, sDiv) * 3;

    nWIdx = pow (2, sDiv + 1) * 3;
    nWVer = pow (2, sDiv + 1) + 2;

    w->nVertices  = (nVer + nWVer) * size;
    w->nIndices   = (nIdx + nWIdx) * size;

    w->nSVer = nVer * size;
    w->nSIdx = nIdx * size;
    w->nWVer = nWVer * size;
    w->nWIdx = nWIdx * size;

    w->size     = size;
    w->distance = distance;
    w->sDiv     = sDiv;

    w->wave1 = 0.0;
    w->wave2 = 0.0;

    w->vertices = calloc (1,sizeof (Vertex) * w->nVertices);
    if (!w->vertices)
    {
	free (w);
	return NULL;
    }

    w->indices = calloc (1,sizeof (int) * w->nIndices);
    if (!w->indices)
    {
	free (w->vertices);
	free (w);
	return NULL;
    }

    r = distance / cos (M_PI / size);
    ang = M_PI / size;
    aStep = 2 * M_PI / size;

    wv = w->vertices + (size * nVer);
    wi = w->indices + (size * nIdx);
    
    for (i = 0; i < size; i++)
    {
	d.v[0] = b.v[0] = sin (ang - aStep) * r;
	d.v[2] = b.v[2] = cos (ang - aStep) * r;

	e.v[0] = c.v[0] = sin (ang) * r;
	e.v[2] = c.v[2] = cos (ang) * r;

	genTriMesh (w->vertices + (i * nVer), w->indices + (i * nIdx),
		    i * nVer, sDiv, a, b, c);

	genTriWall (wv + (i * nWVer / 2), wv + ((i + size) * nWVer / 2),
		    wi + (i * nWIdx), (size * nVer) + (i * nWVer / 2),
		    (size * nVer) + ((i + size) * nWVer / 2), sDiv, b, c, d, e);

	ang += aStep;
    }

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
}

static void
setAmplitude (Vertex *v,
	      float  bh,
	      float  wt,
	      float  swt,
	      float  wa,
	      float  swa,
	      float  wf,
	      float  swf)
{
    float  dx, dz, d, c1, c2;
    Vertex a, b;

    v->v[1] = bh + (wa * sinf (wt + wf * v->v[0] * v->v[2])) +
		   (swa * sinf (swt + swf * v->v[0] * v->v[2]));
    v->v[1] = MIN (0.5, MAX (-0.5, v->v[1]));

    c1 = wa * cosf (wt + wf * v->v[0] * v->v[2]) * wf;
    c2 = swa * cosf (swt + swf * v->v[0] * v->v[2]) * swf;

    dx = (c1 * v->v[2]) + (c2 * v->v[2]);
    dz = (c1 * v->v[0]) + (c2 * v->v[0]);

    a.v[0] = 10;
    a.v[2] = 0;
    b.v[0] = 0;
    b.v[2] = 10;

    a.v[1] = v->v[1] + (10 * dx);
    b.v[1] = v->v[1] + (10 * dz);

    v->n[0] = (b.v[1] * a.v[2]) - (b.v[2] * a.v[1]);
    v->n[1] = (b.v[2] * a.v[0]) - (b.v[0] * a.v[2]);
    v->n[2] = (b.v[0] * a.v[1]) - (b.v[1] * a.v[0]);

    d = sqrt ((v->n[0] * v->n[0]) + (v->n[1] * v->n[1]) + (v->n[2] * v->n[2]));

    v->n[0] /= d;
    v->n[1] /= d;
    v->n[2] /= d;

}

void
updateHeight (Water  *w)
{
    int i;

    if (!w)
	return;

    for (i = 0; i < w->nSVer + (w->nWVer / 2); i++)
        setAmplitude(&w->vertices[i], w->bh, w->wave1, w->wave2, w->wa,
		     w->swa, w->wf, w->swf);
}

void
updateWater (CompScreen *s, float time)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int sDiv = (atlantisGetRenderWaves (s))?
	       atlantisGetGridQuality (s) : 0;
    int size = s->hsize * cs->nOutput;

    if (!as->water)
	as->water = genWater (size, sDiv, cs->distance, -0.5);

    if (!as->water)
	return;

    if (as->water->size != size || as->water->sDiv != sDiv ||
	as->water->distance != cs->distance)
    {
	freeWater (as->water);
	as->water = genWater (size, sDiv, cs->distance, -0.5);

	if (!as->water)
	    return;
    }

    as->water->wave1 += time * atlantisGetWaveSpeed (s);
    as->water->wave2 += time * atlantisGetSmallWaveSpeed (s);

    as->water->wave1 = fmodf (as->water->wave1, 2 * M_PI);
    as->water->wave2 = fmodf (as->water->wave2, 2 * M_PI);

    as->water->bh  = -0.5 + atlantisGetWaterHeight (s);

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

}

void
updateGround (CompScreen *s, float time)
{
    ATLANTIS_SCREEN (s);
    CUBE_SCREEN (s);

    int sDiv = atlantisGetGridQuality (s);
    int size = s->hsize * cs->nOutput;

    Bool update = FALSE;

    if (!as->ground)
    {
	as->ground = genWater (size, sDiv, cs->distance, -0.5);
	update = TRUE;
    }

    if (!as->ground)
	return;

    if (as->ground->size != size || as->ground->sDiv != sDiv ||
	as->ground->distance != cs->distance)
    {
	freeWater (as->ground);
	as->ground = genWater (size, sDiv, cs->distance, -0.5);

	update = TRUE;
	if (!as->ground)
	    return;
    }

    if (!update)
	return;

    as->ground->wave1 = (float)(rand() & 15) / 15.0;
    as->ground->wave2 = (float)(rand() & 15) / 15.0;

    as->ground->bh  = -0.45;
    as->ground->wa  = 0.1;
    as->ground->swa = 0.02;
    as->ground->wf  = 2.0;
    as->ground->swf = 10.0;

    updateHeight (as->ground);
}

void
drawWater (Water *w, Bool full, Bool wire)
{
    static const float mat_shininess[]      = { 50.0 };
    static const float mat_specular[]       = { 0.5, 0.5, 0.5, 1.0 };
    static const float mat_diffuse[]        = { 0.2, 0.2, 0.2, 1.0 };
    static const float mat_ambient[]        = { 0.1, 0.1, 0.1, 1.0 };
    static const float lmodel_ambient[]     = { 0.4, 0.4, 0.4, 1.0 };
    static const float lmodel_localviewer[] = { 0.0 };

    float *v;
    if (!w)
	return;

    glDisable (GL_DEPTH_TEST);

    if (full)
    {
	glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);

	glEnable (GL_COLOR_MATERIAL);
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT1);
	glDisable (GL_LIGHT0);

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	v = (float *) w->vertices;
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState (GL_NORMAL_ARRAY);

	glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);
	glNormalPointer (GL_FLOAT, 6 * sizeof (float), v + 3);
	glDrawElements (GL_TRIANGLES, w->nSIdx, GL_UNSIGNED_INT, w->indices);

	glDisableClientState (GL_NORMAL_ARRAY);
	glDisable (GL_LIGHTING);

	glDrawElements (GL_TRIANGLES, w->nWIdx, GL_UNSIGNED_INT, w->indices + w->nSIdx);

	glEnableClientState (GL_TEXTURE_COORD_ARRAY);	
    }

    if (wire)
    {
	int i, j;

	glColor4usv (defaultColor);

	glDisable (GL_LIGHTING);

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	for (i = 0; i < w->nIndices; i+=3)
	{
	    glBegin(GL_LINE_LOOP);

	    for (j = 0; j < 3; j++)
		glVertex3f(w->vertices[w->indices[i + j]].v[0],
			   w->vertices[w->indices[i + j]].v[1],
			   w->vertices[w->indices[i + j]].v[2]);
	    glEnd();
	}
    }
	
}

void
drawGround (Water *w, Water *g)
{
    static const float mat_shininess[]      = { 40.0 };
    static const float mat_specular[]       = { 0.0, 0.0, 0.0, 1.0 };
    static const float mat_diffuse[]        = { -1.0, -1.0, -1.0, 1.0 };
    static const float mat_ambient[]        = { 0.4, 0.4, 0.4, 1.0 };
    static const float lmodel_ambient[]     = { 0.4, 0.4, 0.4, 1.0 };
    static const float lmodel_localviewer[] = { 0.0 };

    float *v;
    float *n;

    if (!g)
	return;

    glDisable (GL_DEPTH_TEST);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);

    glEnable (GL_COLOR_MATERIAL);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT1);
    glDisable (GL_LIGHT0);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    v = (float *) g->vertices;
    glDisableClientState (GL_TEXTURE_COORD_ARRAY);

    glVertexPointer (3, GL_FLOAT, 6 * sizeof (float), v);

    if (w)
    {
	 n = (float *) w->vertices;
	glEnableClientState (GL_NORMAL_ARRAY);
	glNormalPointer (GL_FLOAT, 6 * sizeof (float), n + 3);
    }
    else
	glNormal3f (0.0, 1.0, 0.0);

    glDrawElements (GL_TRIANGLES, g->nSIdx, GL_UNSIGNED_INT, g->indices);

    glDisableClientState (GL_NORMAL_ARRAY);
    glDisable (GL_LIGHTING);

    glDrawElements (GL_TRIANGLES, g->nWIdx, GL_UNSIGNED_INT, g->indices + g->nSIdx);

    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	
}

void
drawBottomGround (int size, float distance, float bottom)
{
    int   i;
    float r = distance / cos (M_PI / size);
    float ang = M_PI / size;
    float aStep = 2 * M_PI / size;
    
    for (i = 0; i < size; i++)
    {
	glBegin (GL_TRIANGLES);

	glVertex3f (sin (ang - aStep) * r, bottom, cos (ang - aStep) * r);
	glVertex3f (0.0, bottom, 0.0);
	glVertex3f (sin (ang) * r, bottom, cos (ang) * r);
	glEnd ();
	ang += aStep;
    }
}

float
getHeight (Water *w, float x, float z)
{
    if (!w)
	return 0.0;
    return w->bh + (w->wa * sinf (w->wave1 + w->wf * x * z)) +
	   (w->swa * sinf (w->wave2 + w->swf * z * z));
}

