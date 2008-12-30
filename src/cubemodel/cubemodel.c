/*
 * Compiz cube model plugin
 *
 * cubemodel.c
 *
 * This plugin displays wavefront (.obj) 3D mesh models inside of
 * the transparent cube.
 *
 * Copyright : (C) 2008 by David Mikos
 * E-mail    : infiniteloopcounter@gmail.com
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
 * Model loader code based on cubemodel/cubefx plugin "cubemodelModel.c.in"
 * code - originally written by Joel Bosvel (b0le).
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "cubemodel-internal.h"
#include "cubemodel_options.h"

int cubemodelDisplayPrivateIndex;
int cubeDisplayPrivateIndex;

static void
initWorldVariables (CompScreen *s)
{
    CUBEMODEL_SCREEN (s);
    CUBE_SCREEN (s);

    cms->hsize = s->hsize * cs->nOutput;

    cms->arcAngle = 360.0f / cms->hsize;
    cms->radius = cs->distance / cosf (0.5 * (cms->arcAngle * toRadians));
    cms->topDistance = cs->distance;

    if (cubemodelGetRescaleWidth (s))
	cms->ratio = (float) s->width / (float) s->height;
    else
	cms->ratio = 1;

    cms->sideDistance = cms->topDistance * cms->ratio;
}

static void
updateModel (CompScreen *s,
	     int        start,
	     int        end)
{
    int           i;
    CompListValue *modelScale         = cubemodelGetModelScaleFactor (s);
    CompListValue *modelX             = cubemodelGetModelXOffset (s);
    CompListValue *modelY             = cubemodelGetModelYOffset (s);
    CompListValue *modelZ             = cubemodelGetModelZOffset (s);
    CompListValue *modelRotationPlane = cubemodelGetModelRotationPlane (s);
    CompListValue *modelRotationRate  = cubemodelGetModelRotationRate (s);
    CompListValue *modelAnimation     = cubemodelGetModelAnimation (s);
    CompListValue *modelFps           = cubemodelGetModelFps (s);

    CUBEMODEL_SCREEN (s);

    start = MAX (start, 0);
    end   = MIN (end, cms->numModels);

    for (i = start; i< end; i++)
    {
	if (!cms->models[i] || !cms->models[i]->finishedLoading)
	    continue;

	if (modelScale->nValue > i)
	    cms->models[i]->scaleGlobal = modelScale->value[i].f;

	if (modelX->nValue > i)
	    cms->models[i]->translate[0] = modelX->value[i].f * cms->ratio;
	if (modelY->nValue > i)
	    cms->models[i]->translate[1] = modelY->value[i].f;
	if (modelZ->nValue > i)
	    cms->models[i]->translate[2] = modelZ->value[i].f * cms->ratio;

	if (modelRotationPlane->nValue > i)
	{
	    int rot = modelRotationPlane->value[i].i;

	    switch (rot % 3) {
	    case 0:
		cms->models[i]->rotate[1] = 0;
		cms->models[i]->rotate[2] = 1;
		cms->models[i]->rotate[3] = 0;
	    	break;
	    case 1:
		cms->models[i]->rotate[1] = 1;
		cms->models[i]->rotate[2] = 0;
		cms->models[i]->rotate[3] = 0;
	    	break;
	    case 2:
		cms->models[i]->rotate[1] = 0;
		cms->models[i]->rotate[2] = 0;
		cms->models[i]->rotate[3] = 1;
	    	break;
	    }

	    if (rot / 3 != 0)
	    {
		cms->models[i]->rotate[1] *= -1;
		cms->models[i]->rotate[2] *= -1;
		cms->models[i]->rotate[3] *= -1;
	    }

	}
	if (modelRotationRate->nValue > i)
	    cms->models[i]->rotateSpeed = modelRotationRate->value[i].f;

	if (modelFps->nValue > i)
	{
	    cms->models[i]->fps = modelFps->value[i].i;

	    if (modelAnimation->nValue > i && modelAnimation->value[i].i == 2)
		cms->models[i]->fps *= -1;
	}
    }
}

static void
initCubemodel (CompScreen *s)
{
    int   i, numModels;
    float translate[] = { 0, 0, 0 };
    float rotate[]    = { 0, 0, 0, 0 };
    float scale[]     = { 1, 1, 1, 1 };
    float color[]     = { 1, 1, 1, 1 };
    float rotateSpeed = 0;
    Bool  status, animation = FALSE;
    int   fps = 3;

    CUBEMODEL_SCREEN (s);

    CompListValue * modelFilename      = cubemodelGetModelFilename (s);
    CompListValue * modelScale         = cubemodelGetModelScaleFactor (s);
    CompListValue * modelX             = cubemodelGetModelXOffset (s);
    CompListValue * modelY             = cubemodelGetModelYOffset (s);
    CompListValue * modelZ             = cubemodelGetModelZOffset (s);
    CompListValue * modelRotationPlane = cubemodelGetModelRotationPlane (s);
    CompListValue * modelRotationRate  = cubemodelGetModelRotationRate (s);
    CompListValue * modelAnimation     = cubemodelGetModelAnimation (s);
    CompListValue * modelFps           = cubemodelGetModelFps (s);

    numModels = modelFilename->nValue;

    if (modelScale->nValue < numModels)
	numModels = modelScale->nValue;

    if (modelX->nValue < numModels)
	numModels = modelX->nValue;
    if (modelY->nValue < numModels)
	numModels = modelY->nValue;
    if (modelZ->nValue < numModels)
	numModels = modelZ->nValue;
    if (modelRotationPlane->nValue < numModels)
	numModels = modelRotationPlane->nValue;
    if (modelRotationRate->nValue < numModels)
	numModels = modelRotationRate->nValue;
    if (modelAnimation->nValue < numModels)
	numModels = modelAnimation->nValue;
    if (modelFps->nValue < numModels)
	numModels = modelFps->nValue;

    cms->numModels = numModels;
    cms->modelFilename = malloc (numModels * sizeof(char *));

    cms->models = malloc (numModels * sizeof (CubemodelObject *));

    for (i = 0; i < numModels; i++)
	cms->models[i] = NULL;

    for (i = 0; i < numModels; i++)
    {
	cms->models[i] = malloc (sizeof (CubemodelObject));
	if (!cms->models[i])
	    break;

	animation = FALSE;
	if (modelAnimation->value[i].i > 0)
	    animation = TRUE;

	status = FALSE;

	if (modelFilename->nValue > i)
	    status = cubemodelAddModelObject (s, cms->models[i],
					      modelFilename->value[i].s,
					      translate, rotate,
					      rotateSpeed, scale,
					      color, animation, fps);

	if (!status)
	    cms->modelFilename[i] = NULL;
	else
	    cms->modelFilename[i] = strdup (modelFilename->value[i].s);
    }

    updateModel (s, 0, cms->numModels);

    initWorldVariables (s);
}

static void
freeCubemodel (CompScreen *s)
{
    CUBEMODEL_SCREEN (s);

    int i;

    if (cms->models)
    {
	for (i = 0; i < cms->numModels; i++)
	{
	    if (cms->models[i])
	    {
		cubemodelDeleteModelObject (s, cms->models[i]);
		free (cms->models[i]);
	    }
	}
	free (cms->models);
    }

    if (cms->modelFilename)
    {
	for (i = 0; i < cms->numModels; i++)
	{
	    if (cms->modelFilename[i])
		free (cms->modelFilename[i]);
	}

	free (cms->modelFilename);
    }
}

static void
updateCubemodel (CompScreen *s)
{
    freeCubemodel (s);
    initCubemodel (s);
}

static void
cubemodelLoadingOptionChange (CompScreen             *s,
			      CompOption             *opt,
			      CubemodelScreenOptions num)
{
    int           i, numModels, fps = 3;
    CompListValue *modelFilename, *modelAnimation;
    float         translate[] = { 0, 0, 0 };
    float         rotate[]    = { 0, 0, 0, 0 };
    float         scale[]     = { 1, 1, 1, 1 };
    float         color[]     = { 1, 1, 1, 1 };
    float         rotateSpeed = 0;

    CUBEMODEL_SCREEN (s);

    modelFilename  = cubemodelGetModelFilename (s);
    modelAnimation = cubemodelGetModelAnimation (s);
    numModels      = modelAnimation->nValue;

    if (modelAnimation->nValue < numModels)
	numModels = modelAnimation->nValue;

    if (!cms->models || cms->numModels <= 0 || !cms->modelFilename)
    {
	updateCubemodel (s);
	return;
    }


    if (numModels != cms->numModels)
    {
	for (i = numModels; i < cms->numModels; i++)
	{
	    cubemodelDeleteModelObject (s, cms->models[i]);

	    if (cms->modelFilename[i])
		free (cms->modelFilename[i]);
	    if (cms->models[i])
		free (cms->models[i]);
	}

	cms->modelFilename = realloc (cms->modelFilename,
				      numModels * sizeof (char *));
	cms->models        = realloc (cms->models,
				      numModels * sizeof (CubemodelObject *));

	for (i = cms->numModels; i < numModels; i++)
	{
	    cms->modelFilename[i] = NULL;
	    cms->models[i] = malloc (sizeof (CubemodelObject));
	}

	cms->numModels = numModels;
    }

    for (i = 0; i < numModels; i++)
    {
	Bool animation, fileDiff = TRUE;

	if (!modelFilename->value)
	    continue;

	if (modelFilename->nValue <= i)
	    continue;

	if (!cms->modelFilename[i] && !modelFilename->value[i].s)
	    continue;

	if (cms->modelFilename[i] && modelFilename->value[i].s)
	    fileDiff = strcmp (cms->modelFilename[i],
			       modelFilename->value[i].s);

	animation = (modelAnimation->value[i].i > 0);

	if (animation != cms->models[i]->animation || fileDiff)
	{
	    Bool status;

	    cubemodelDeleteModelObject (s, cms->models[i]);
	    if (cms->modelFilename[i])
		free (cms->modelFilename[i]);

	    status = cubemodelAddModelObject (s, cms->models[i],
					      modelFilename->value[i].s,
					      translate, rotate, rotateSpeed,
					      scale, color, animation, fps);
	    if (!status)
		cms->modelFilename[i] = NULL;
	    else
		cms->modelFilename[i] = strdup (modelFilename->value[i].s);
	}
    }

    updateModel (s, 0, cms->numModels);
}

static void
cubemodelModelOptionChange (CompScreen             *s,
			    CompOption             *opt,
			    CubemodelScreenOptions num)
{
    CUBEMODEL_SCREEN (s);

    if (!cms->models || cms->numModels <= 0)
    {
	updateCubemodel (s);
	return;
    }

    updateModel (s, 0, cms->numModels);
}


static void
cubemodelClearTargetOutput (CompScreen *s,
			    float      xRotate,
			    float      vRotate)
{
    CUBEMODEL_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (cms, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, vRotate);
    WRAP (cms, cs, clearTargetOutput, cubemodelClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static void
setLightPosition (CompScreen *s,
		  GLenum     light)
{
    float angle = cubemodelGetLightInclination (s) * toRadians;
    float position[] = { 0.0, 0.0, 1.0, 0.0 };

    if (cubemodelGetRotateLighting (s))
	angle = 0;

    position[1] = sinf (angle);
    position[2] = cosf (angle);

    glLightfv (light, GL_POSITION, position);
}

static void
cubemodelPaintInside (CompScreen              *s,
                      const ScreenPaintAttrib *sAttrib,
                      const CompTransform     *transform,
                      CompOutput              *output,
                      int                     size)
{
    int                i;
    static const float matShininess[] = { 60.0 };
    static const float matSpecular[] = { 0.6, 0.6, 0.6, 1.0 };
    static const float matDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float matAmbient[] = { 0.8, 0.8, 0.9, 1.0 };

    static const float lmodelLocalviewer[] = { 0.0 };
    static       float lmodelTwoside[] = { 0.0 };
    static       float lmodelAmbient[] = { 0.4, 0.4, 0.4, 0.4 };
    static       float lmodelDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static       float lmodelSpecular[]= { 0.6, 0.6, 0.6, 0.6 };

    ScreenPaintAttrib  sA = *sAttrib;
    CompTransform      mT = *transform;
    Bool               enabledCull;
    int                cull;
    float              scale, outputRatio = 1.0f;

    CUBEMODEL_SCREEN (s);
    CUBE_SCREEN (s);

    if (cms->hsize != s->hsize * cs->nOutput)
    {
	initWorldVariables (s);
	updateModel (s, 0, cms->numModels);
    }

    sA.yRotate += cs->invert * (360.0f / size) *
		  (cs->xRotations - (s->x* cs->nOutput));

    (*s->applyScreenTransform) (s, &sA, output, &mT);

    glPushMatrix ();

    if (cubemodelGetRotateLighting (s))
	setLightPosition (s, GL_LIGHT1);

    glLoadMatrixf (mT.m);

    if (!cubemodelGetRotateLighting (s))
	setLightPosition (s, GL_LIGHT1);


    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);
    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT |
		  GL_LIGHTING_BIT     | GL_DEPTH_BUFFER_BIT);

    glEnable (GL_BLEND);
    glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    lmodelAmbient[0]  = cubemodelGetLightAmbient (s);
    lmodelDiffuse[0]  = cubemodelGetLightDiffuse (s);
    lmodelSpecular[0] = cubemodelGetLightSpecular (s);

    for (i = 1; i < 4; i++)
    {
	lmodelAmbient[i]  = lmodelAmbient[0];
	lmodelDiffuse[i]  = lmodelDiffuse[0];
	lmodelSpecular[i] = lmodelSpecular[0];
    }

    lmodelTwoside[0] = (cubemodelGetRenderFrontAndBack (s) ? 1.0f : 0.0f);


    glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodelLocalviewer);
    glLightModelfv (GL_LIGHT_MODEL_TWO_SIDE, lmodelTwoside);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT,  lmodelAmbient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE,   lmodelDiffuse);
    glLightfv (GL_LIGHT1, GL_SPECULAR,  lmodelSpecular);

    enabledCull = glIsEnabled (GL_CULL_FACE);

    glGetIntegerv (GL_CULL_FACE_MODE, &cull);
    glEnable (GL_CULL_FACE);

    glCullFace (~cull & (GL_FRONT | GL_BACK));
    glCullFace (cull);

    glPushMatrix ();

    glColor4usv (defaultColor);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, matShininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);

    glEnable (GL_NORMALIZE);
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_COLOR_MATERIAL);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT1);
    glDisable (GL_LIGHT0);

    glDepthFunc (GL_LEQUAL); /* for transparency maps */
    glShadeModel(GL_SMOOTH);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    scale = cs->distance;

    if (cubemodelGetRescaleWidth (s))
    {
	if (cs->moMode == CUBE_MOMODE_AUTO && cs->nOutput < s->nOutputDev)
	    outputRatio = (float) s->width / (float) s->height;
	else
	    outputRatio = (float) output->width / (float) output->height;
    }

    glScalef (scale / outputRatio, scale, scale / outputRatio);

    glPushMatrix ();

    glColor4f (1.0, 1.0, 1.0, 1.0);

    for (i = 0; i < cms->numModels; i++)
    {
	glPushMatrix ();
	cubemodelDrawModelObject (s, cms->models[i],
	                          cubemodelGetGlobalModelScaleFactor (s));
	glPopMatrix ();

    }
    glPopMatrix ();

    glPopMatrix ();

    glDisable (GL_LIGHT1);
    glDisable (GL_NORMALIZE);

    if (!s->lighting)
	glDisable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

    glPopMatrix ();

    glPopAttrib ();

    cms->damage = TRUE;

    UNWRAP (cms, cs, paintInside);
    (*cs->paintInside) (s, sAttrib, transform, output, size);
    WRAP (cms, cs, paintInside, cubemodelPaintInside);
}

static void
cubemodelPreparePaintScreen (CompScreen *s,
			     int        ms)
{
    int i;

    CUBEMODEL_SCREEN (s);

    for (i = 0; i < cms->numModels; i++)
    {
	if (!cms->models[i]->finishedLoading)
	    continue;

	if (cms->models[i]->updateAttributes)
	{
	    updateModel (s, i, i + 1);
	    cms->models[i]->updateAttributes = FALSE;
	}

	cubemodelUpdateModelObject (s, cms->models[i],  ms / 1000.0f);
    }

    UNWRAP (cms, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (cms, s, preparePaintScreen, cubemodelPreparePaintScreen);
}

static void
cubemodelDonePaintScreen (CompScreen *s)
{
    CUBEMODEL_SCREEN (s);

    if (cms->damage)
    {
	damageScreen (s);
	cms->damage = FALSE;
    }

    UNWRAP (cms, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (cms, s, donePaintScreen, cubemodelDonePaintScreen);
}

static Bool
cubemodelInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    CubemodelDisplay *cmd;

    if (!checkPluginABI ("core", CORE_ABIVERSION)
	|| !checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    cmd = malloc (sizeof (CubemodelDisplay));
    if (!cmd)
	return FALSE;

    cmd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (cmd->screenPrivateIndex < 0)
    {
	free (cmd);
	return FALSE;
    }

    d->base.privates[cubemodelDisplayPrivateIndex].ptr = cmd;

    return TRUE;
}

static void
cubemodelFiniDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    CUBEMODEL_DISPLAY (d);

    freeScreenPrivateIndex (d, cmd->screenPrivateIndex);
    free (cmd);
}

static Bool
cubemodelInitScreen (CompPlugin *p,
		CompScreen *s)
{
    static const float ambient[]  = { 0.0, 0.0, 0.0, 0.0 };
    static const float diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
    static const float specular[] = { 0.6, 0.6, 0.6, 1.0 };

    CubemodelScreen *cms;

    CUBEMODEL_DISPLAY (s->display);
    CUBE_SCREEN (s);

    cms = malloc (sizeof (CubemodelScreen));
    if (!cms)
	return FALSE;

    s->base.privates[cmd->screenPrivateIndex].ptr = cms;

    cms->damage = FALSE;

    glLightfv (GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT1, GL_SPECULAR, specular);

    initCubemodel (s);

    cubemodelSetModelFilenameNotify      (s, cubemodelLoadingOptionChange);
    cubemodelSetModelAnimationNotify     (s, cubemodelLoadingOptionChange);

    cubemodelSetModelScaleFactorNotify   (s, cubemodelModelOptionChange);
    cubemodelSetModelXOffsetNotify       (s, cubemodelModelOptionChange);
    cubemodelSetModelYOffsetNotify       (s, cubemodelModelOptionChange);
    cubemodelSetModelZOffsetNotify       (s, cubemodelModelOptionChange);
    cubemodelSetModelRotationPlaneNotify (s, cubemodelModelOptionChange);
    cubemodelSetModelRotationRateNotify  (s, cubemodelModelOptionChange);
    cubemodelSetModelFpsNotify           (s, cubemodelModelOptionChange);
    cubemodelSetRescaleWidthNotify       (s, cubemodelModelOptionChange);

    WRAP (cms, s, donePaintScreen, cubemodelDonePaintScreen);
    WRAP (cms, s, preparePaintScreen, cubemodelPreparePaintScreen);
    WRAP (cms, cs, clearTargetOutput, cubemodelClearTargetOutput);
    WRAP (cms, cs, paintInside, cubemodelPaintInside);

    return TRUE;
}

static void
cubemodelFiniScreen (CompPlugin *p,
		     CompScreen *s)
{
    CUBEMODEL_SCREEN (s);
    CUBE_SCREEN (s);

    freeCubemodel (s);

    UNWRAP (cms, s, donePaintScreen);
    UNWRAP (cms, s, preparePaintScreen);
    UNWRAP (cms, cs, clearTargetOutput);
    UNWRAP (cms, cs, paintInside);

    free (cms);
}

static Bool
cubemodelInit (CompPlugin *p)
{
    cubemodelDisplayPrivateIndex = allocateDisplayPrivateIndex ();

    if (cubemodelDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
cubemodelFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (cubemodelDisplayPrivateIndex);
}

static CompBool
cubemodelInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) cubemodelInitDisplay,
	(InitPluginObjectProc) cubemodelInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
cubemodelFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) cubemodelFiniDisplay,
	(FiniPluginObjectProc) cubemodelFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable cubemodelVTable = {
    "cubemodel",
    0,
    cubemodelInit,
    cubemodelFini,
    cubemodelInitObject,
    cubemodelFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &cubemodelVTable;
}
