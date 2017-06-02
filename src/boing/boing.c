/**
 *
 * Compiz Boing plugin
 *
 * boing.c
 * 
 * Boing was adapted from the Snow plugin by Patrick Fisher. The copywrites and warranty 
 * from the Snow plugin remain in effect, as shown below.
 *
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 * Snow plugin maintained by Danny Baumann <maniac@opencompositing.org>
 * Boing Plugin maintained by Patrick Fisher <boingplugin@gmail.com>
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
 **/

/*
 * Many thanks to Atie H. <atie.at.matrix@gmail.com> for providing
 * a clean plugin template
 * Also thanks to the folks from #beryl-dev, especially Quinn_Storm
 * for helping me make this possible
 */

#include <compiz-core.h>
#include "boing_options.h"


#define GET_BOING_DISPLAY(d)                            \
    ((BoingDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define BOING_DISPLAY(d)                                \
    BoingDisplay *sd = GET_BOING_DISPLAY (d)

#define GET_BOING_SCREEN(s, sd)                         \
    ((LeafScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)

#define BOING_SCREEN(s)                                 \
    LeafScreen *ss = GET_BOING_SCREEN (s, GET_BOING_DISPLAY (s->display))

static int displayPrivateIndex = 0;

/* -------------------  STRUCTS ----------------------------- */
typedef struct _BoingDisplay
{
    int screenPrivateIndex;

    Bool useTextures;

    int             boingTexNFiles;
    CompOptionValue *boingTexFiles;
} BoingDisplay;

typedef struct _LeafTexture
{
    CompTexture tex;

    unsigned int width;
    unsigned int height;

    Bool   loaded;
    GLuint dList;
} LeafTexture;

typedef struct _Leaf
{
    float x, y, z;
    float xs, ys, zs;
    float ra; /* rotation angle */
    float rs; /* rotation speed */
    int frame;
    float dx;
    float dy;
    float velocityY;
    float velocityX;
    float spin;
    float spinspeed;
    float tempframe;

    LeafTexture *tex;
} Leaf;

typedef struct _LeafScreen
{
    CompScreen *s;

    Bool active;

    CompTimeoutHandle timeoutHandle;

    PaintOutputProc paintOutput;
    DrawWindowProc  drawWindow;

    LeafTexture *boingTex;
    int         boingTexturesLoaded;

    GLuint displayList;
    Bool   displayListNeedsUpdate;

    Leaf *allLeaves;
} LeafScreen;

/* some forward declarations */
static void initiateLeaf (LeafScreen * ss, Leaf * sf);

static void
boingThink (LeafScreen *ss,
	   Leaf  *sf)
{
sf->x = sf->x + sf->dx;
sf->velocityY=sf->velocityY+0.1;
sf->y = sf->y + sf->velocityY;

    if (sf->y >= ss->s->height)
	{
	sf->y = ss->s->height-2;
	sf->dy = sf->dy*-1;
        sf->velocityY=-sf->velocityY;
	}
    if (sf->x <= 1)
	{
	sf->x=2;
	sf->dx = sf->dx*-1;
	}
    if (sf->x >= ss->s->width)
	{
	sf->x=ss->s->width-2;
	sf->dx = sf->dx*-1;
	}
    if (sf->y <= 1)
	{
	sf->y=2;
	sf->dy = sf->dy * -1;
	sf->velocityY=-sf->velocityY;
	}

    sf->tempframe=sf->tempframe+sf->spin;
    sf->frame=sf->tempframe;
    if (sf->frame > ss->boingTexturesLoaded-1)
{
	sf->frame = 0;
        sf->tempframe = 0;
}
    if (sf->frame < 0)
{
        sf->frame = ss->boingTexturesLoaded-1;
        sf->tempframe = ss->boingTexturesLoaded-1;
}
    sf->tex = &ss->boingTex[sf->frame];
}

static Bool
stepLeafPositions (void *closure)
{
    CompScreen *s = closure;
    int        i, numLeaves;
    Leaf  *boingLeaf;
    Bool       onTop;

    BOING_SCREEN (s);

    if (!ss->active)
	return TRUE;

    boingLeaf = ss->allLeaves;
    numLeaves = boingGetNumLeaves (s->display);
    onTop = boingGetLeavesOverWindows (s->display);

    for (i = 0; i < numLeaves; i++)
	boingThink(ss, boingLeaf++);

    if (ss->active && !onTop)
    {
	CompWindow *w;

	for (w = s->windows; w; w = w->next)
	{
	    if (w->type & CompWindowTypeDesktopMask)
		addWindowDamage (w);
	}
    }
    else if (ss->active)
	damageScreen (s);

    return TRUE;
}

static Bool
boingToggle (CompDisplay     *d,
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
	BOING_SCREEN (s);
	ss->active = !ss->active;
	if (!ss->active)
	    damageScreen (s);
    }

    return TRUE;
}

/* --------------------  HELPER FUNCTIONS ------------------------ */

static inline int
getRand (int min,
	 int max)
{

    return (rand() % (max - min + 1) + min);
}

static inline float
mmRand (int   min,
	int   max,
	float divisor)
{
    return ((float) getRand(min, max)) / divisor;
};

/* --------------------------- RENDERING ------------------------- */
static void
setupDisplayList (LeafScreen *ss)
{
    float boingSize = boingGetLeafSize (ss->s->display);

    ss->displayList = glGenLists (1);

    glNewList (ss->displayList, GL_COMPILE);
    glBegin (GL_QUADS);

    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, 0, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, boingSize, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (boingSize, boingSize, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (boingSize, 0, -0.0);

    glEnd ();
    glEndList ();
}

static void
beginRendering (LeafScreen *ss,
		CompScreen *s)
{

    if (boingGetUseBlending (s->display))
	glEnable (GL_BLEND);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (ss->displayListNeedsUpdate)
    {
	setupDisplayList (ss);
	ss->displayListNeedsUpdate = FALSE;
    }

    glColor4f (1.0, 1.0, 1.0, 1.0);
    if (ss->boingTexturesLoaded && boingGetUseTextures (s->display))
    {
	int j;

	for (j = 0; j < ss->boingTexturesLoaded; j++)
	{
	    Leaf *boingLeaf = ss->allLeaves;
	    int       i, numLeaves = boingGetNumLeaves (s->display);
	    Bool      boingRotate = boingGetLeafRotation (s->display);

	    enableTexture (ss->s, &ss->boingTex[j].tex,
			   COMP_TEXTURE_FILTER_GOOD);

	    for (i = 0; i < numLeaves; i += 2)
	    {
		if (boingLeaf->tex == &ss->boingTex[j])
		{
		    glTranslatef (boingLeaf->x, boingLeaf->y, boingLeaf->z);
		
	    	    if (boingRotate)
			glRotatef (boingLeaf->ra, 0, 0, 1);
	    	    glCallList (ss->boingTex[j].dList);
    		    if (boingRotate)
			glRotatef (-boingLeaf->ra, 0, 0, 1);
	    	    glTranslatef (-boingLeaf->x, -boingLeaf->y, -boingLeaf->z);
		
		}
		boingLeaf++;
	    }

	    for (i = 1; i < (numLeaves); i += 2)
	    {
		if (boingLeaf->tex == &ss->boingTex[j])
		{
		    glTranslatef (boingLeaf->x, boingLeaf->y, boingLeaf->z);
		
	    	    if (boingRotate)
			glRotatef (-boingLeaf->ra, 0, 0, 1);
	    	    glCallList (ss->boingTex[j].dList);
    		    if (boingRotate)
			glRotatef (boingLeaf->ra, 0, 0, 1);
	    	    glTranslatef (-boingLeaf->x, -boingLeaf->y, -boingLeaf->z);
		
		}
		boingLeaf++;
	    }
	    disableTexture (ss->s, &ss->boingTex[j].tex);
	}
    }
    else
    {
	Leaf *boingLeaf = ss->allLeaves;
	int       i, numLeaves = boingGetNumLeaves (s->display);

	for (i = 0; i < numLeaves; i++)
	{
	    glTranslatef (boingLeaf->x, boingLeaf->y, boingLeaf->z);
	    glRotatef (boingLeaf->ra, 0, 0, 1);
	    glCallList (ss->displayList);
	    glRotatef (-boingLeaf->ra, 0, 0, 1);
	    glTranslatef (-boingLeaf->x, -boingLeaf->y, -boingLeaf->z);
	    boingLeaf++;
	}
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    if (boingGetUseBlending (s->display))
    {
	glDisable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

/* ------------------------  FUNCTIONS -------------------- */

static Bool
boingPaintOutput (CompScreen              *s,
		 const ScreenPaintAttrib *sa,
		 const CompTransform	 *transform,
		 Region                  region,
		 CompOutput              *output, 
		 unsigned int            mask)
{
    Bool status;

    BOING_SCREEN (s);

    if (ss->active && !boingGetLeavesOverWindows (s->display))
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (ss, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ss, s, paintOutput, boingPaintOutput);

    if (ss->active && boingGetLeavesOverWindows (s->display))
    {
	CompTransform sTransform = *transform;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	glPushMatrix ();
	glLoadMatrixf (sTransform.m);
	beginRendering (ss, s);
	glPopMatrix ();
    }

    return status;
}

static Bool
boingDrawWindow (CompWindow           *w,
		const CompTransform  *transform,
		const FragmentAttrib *attrib,
		Region               region,
		unsigned int         mask)
{
    Bool status;

    BOING_SCREEN (w->screen);

    /* First draw Window as usual */
    UNWRAP (ss, w->screen, drawWindow);
    status = (*w->screen->drawWindow) (w, transform, attrib, region, mask);
    WRAP (ss, w->screen, drawWindow, boingDrawWindow);

    /* Check whether this is the Desktop Window */
    if (ss->active && (w->type & CompWindowTypeDesktopMask) && 
	!boingGetLeavesOverWindows (w->screen->display))
    {
	beginRendering (ss, w->screen);
    }

    return status;
}

static void
initiateLeaf (LeafScreen *ss,
		   Leaf  *sf)
{
    int boxing = boingGetScreenBoxing (ss->s->display);

    switch (boingGetWindDirection (ss->s->display))
    {
    case WindDirectionNoWind:
	sf->x  = mmRand (-boxing, ss->s->width + boxing, 1);
	sf->xs = mmRand (-3, 3, 1);
	sf->y  = mmRand (-300, 0, 1);
	sf->ys = mmRand (1, 3, 1);
	break;
    case WindDirectionUp:
	sf->x  = mmRand (-boxing, ss->s->width + boxing, 1);
	sf->xs = mmRand (-3, 3, 1);
	sf->y  = mmRand (ss->s->height, ss->s->height + 300, 1);
	sf->ys = -mmRand (1, 3, 1);
	break;
    case WindDirectionLeft:
	sf->x  = mmRand (ss->s->width, ss->s->width + 300, 1);
	sf->xs = -mmRand (1, 3, 1);
	sf->y  = mmRand (-boxing, ss->s->height + boxing, 1);
	sf->ys = mmRand (-3, 3, 1);
	break;
    case WindDirectionRight:
	sf->x  = mmRand (-300, 0, 1);
	sf->xs = mmRand (1, 3, 1);
	sf->y  = mmRand (-boxing, ss->s->height + boxing, 1);
	sf->ys = mmRand (-3, 3, 1);
	break;
    default:
	break;
    }

    sf->z  = mmRand (-boingGetScreenDepth (ss->s->display), 0.1, 5000);
    sf->zs = mmRand (-1000, 1000, 500000);
    sf->ra = mmRand (-1000, 1000, 50);
    sf->rs = mmRand (-2100, 2100, 700);

    sf->dx = getRand (-3,3);
    if (sf->dx<1 && sf->dx>-1)
	sf->dx=sf->dx+0.8;
    sf->dy = getRand (-8,8);
    if (sf->dy <1 && sf->dy>-1)
	sf->dy=sf->dy+0.8;
    sf->y = getRand (200,800);
    sf->x = getRand (200,800);
    sf->velocityY=getRand (.5,1.5);
    sf->velocityX=getRand (.5,1.5);

    sf->spin=getRand (0 , 10);
    if (sf->spin < 5)
          sf->spin=-1;
    if (sf->spin >= 5)
          sf->spin=1;
    sf->spinspeed=getRand (.2, 1);
    if (sf->spin==-1)
          sf->spinspeed=-sf->spinspeed;
    sf->spin=sf->spin+sf->spinspeed;
}

static void
setLeafTexture (LeafScreen *ss,
		     Leaf  *sf)
{
    if (ss->boingTexturesLoaded)
	sf->tex = &ss->boingTex[rand () % ss->boingTexturesLoaded];
}

static void
updateLeafTextures (CompScreen *s)
{
    int       i, count = 0;
    float     boingSize = boingGetLeafSize(s->display);
    int       numLeaves = boingGetNumLeaves(s->display);
    Leaf *boingLeaf;

    BOING_SCREEN (s);
    BOING_DISPLAY (s->display);

    boingLeaf = ss->allLeaves;

    for (i = 0; i < ss->boingTexturesLoaded; i++)
    {
	finiTexture (s, &ss->boingTex[i].tex);
	glDeleteLists (ss->boingTex[i].dList, 1);
    }

    if (ss->boingTex)
	free (ss->boingTex);
    ss->boingTexturesLoaded = 0;

    ss->boingTex = calloc (1, sizeof (LeafTexture) * sd->boingTexNFiles);

    for (i = 0; i < sd->boingTexNFiles; i++)
    {
	CompMatrix  *mat;
	LeafTexture *sTex;

	ss->boingTex[count].loaded =
	    readImageToTexture (s, &ss->boingTex[count].tex,
				sd->boingTexFiles[i].s,
				&ss->boingTex[count].width,
				&ss->boingTex[count].height);
	if (!ss->boingTex[count].loaded)
	{
	    compLogMessage ("boing", CompLogLevelWarn,
			    "Texture not found : %s", sd->boingTexFiles[i].s);
	    continue;
	}
	compLogMessage ("boing", CompLogLevelInfo,
			"Loaded Texture %s", sd->boingTexFiles[i].s);
	
	mat = &ss->boingTex[count].tex.matrix;
	sTex = &ss->boingTex[count];

	sTex->dList = glGenLists (1);
	glNewList (sTex->dList, GL_COMPILE);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
	glVertex2f (0, 0);
	glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, sTex->height));
	glVertex2f (0, boingSize * sTex->height / sTex->width);
	glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->width),
		      COMP_TEX_COORD_Y (mat, sTex->height));
	glVertex2f (boingSize, boingSize * sTex->height / sTex->width);
	glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->width),
		      COMP_TEX_COORD_Y (mat, 0));
	glVertex2f (boingSize, 0);

	glEnd ();
	glEndList ();

	count++;
    }

    ss->boingTexturesLoaded = count;
    if (count < sd->boingTexNFiles)
	ss->boingTex = realloc (ss->boingTex, sizeof (LeafTexture) * count);

    for (i = 0; i < numLeaves; i++)
	setLeafTexture (ss, boingLeaf++);
}

static Bool
boingInitScreen (CompPlugin *p,
		CompScreen *s)
{
    LeafScreen *ss;
    int        i, numLeaves = boingGetNumLeaves (s->display);
    Leaf  *boingLeaf;

    BOING_DISPLAY (s->display);

    ss = calloc (1, sizeof(LeafScreen));

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    ss->s = s;
    ss->boingTexturesLoaded = 0;
    ss->boingTex = NULL;
    ss->active = FALSE;
    ss->displayListNeedsUpdate = FALSE;

    ss->allLeaves = boingLeaf = malloc (numLeaves * sizeof (Leaf));

    for (i = 0; i < numLeaves; i++)
    {
	initiateLeaf (ss, boingLeaf);
	setLeafTexture (ss, boingLeaf);
	boingLeaf++;
    }

    updateLeafTextures (s);
    setupDisplayList (ss);

    WRAP (ss, s, paintOutput, boingPaintOutput);
    WRAP (ss, s, drawWindow, boingDrawWindow);

    ss->timeoutHandle = compAddTimeout (boingGetAutumnUpdateDelay (s->display),
					boingGetAutumnUpdateDelay (s->display),
					stepLeafPositions, s);

    return TRUE;
}

static void
boingFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    int i;

    BOING_SCREEN (s);

    if (ss->timeoutHandle)
	compRemoveTimeout (ss->timeoutHandle);

    for (i = 0; i < ss->boingTexturesLoaded; i++)
    {
	finiTexture (s, &ss->boingTex[i].tex);
	glDeleteLists (ss->boingTex[i].dList, 1);
    }

    if (ss->boingTex)
	free (ss->boingTex);

    if (ss->allLeaves)
	free (ss->allLeaves);

    UNWRAP (ss, s, paintOutput);
    UNWRAP (ss, s, drawWindow);

    free (ss);
}

static void
boingDisplayOptionChanged (CompDisplay        *d,
			  CompOption         *opt,
			  BoingDisplayOptions num)
{
    BOING_DISPLAY (d);

    switch (num)
    {
    case BoingDisplayOptionLeafSize:
	{
	    CompScreen *s;

	    for (s = d->screens; s; s = s->next)
	    {
		BOING_SCREEN (s);
		ss->displayListNeedsUpdate = TRUE;
		updateLeafTextures (s);
	    }
	}
	break;
    case BoingDisplayOptionAutumnUpdateDelay:
	{
	    CompScreen *s;

	    for (s = d->screens; s; s = s->next)
	    {
		BOING_SCREEN (s);
					
		if (ss->timeoutHandle)
		    compRemoveTimeout (ss->timeoutHandle);
		ss->timeoutHandle =
		    compAddTimeout (boingGetAutumnUpdateDelay (d),
				    boingGetAutumnUpdateDelay (d),
				    stepLeafPositions, s);
	    }
	}
	break;
    case BoingDisplayOptionNumLeaves:
	{
	    CompScreen *s;
	    int        i, numLeaves;
	    Leaf  *boingLeaf;

	    numLeaves = boingGetNumLeaves (d);
	    for (s = d->screens; s; s = s->next)
    	    {
		BOING_SCREEN (s);
		ss->allLeaves = realloc (ss->allLeaves,
					     numLeaves * sizeof (Leaf));
		boingLeaf = ss->allLeaves;

		for (i = 0; i < numLeaves; i++)
		{
		    initiateLeaf (ss, boingLeaf);
    		    setLeafTexture (ss, boingLeaf);
	    	    boingLeaf++;
		}
	    }
	}
	break;
    case BoingDisplayOptionLeafTextures:
	{
	    CompScreen *s;
	    CompOption *texOpt;

	    texOpt = boingGetLeafTexturesOption (d);

	    sd->boingTexFiles = texOpt->value.list.value;
    	    sd->boingTexNFiles = texOpt->value.list.nValue;

	    for (s = d->screens; s; s = s->next)
		updateLeafTextures (s);
	}
	break;
    case BoingDisplayOptionDefaultEnabled:
	{
	    CompScreen *s;
	    for (s = d->screens; s; s = s->next)
	    {
		BOING_SCREEN (s);
		ss->active = boingGetDefaultEnabled(s->display);
		ss->displayListNeedsUpdate = TRUE;
		damageScreen (s);
	    }
	}
	break;
    default:
	break;
    }
}

static Bool
boingInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    CompOption  *texOpt;
    BoingDisplay *sd;
    
    sd = malloc (sizeof (BoingDisplay));

    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (sd->screenPrivateIndex < 0)
    {
	free (sd);
	return FALSE;
    }
	
    boingSetToggleKeyInitiate (d, boingToggle);
    boingSetNumLeavesNotify (d, boingDisplayOptionChanged);
    boingSetLeafSizeNotify (d, boingDisplayOptionChanged);
    boingSetAutumnUpdateDelayNotify (d, boingDisplayOptionChanged);
    boingSetLeafTexturesNotify (d, boingDisplayOptionChanged);
    boingSetDefaultEnabledNotify (d, boingDisplayOptionChanged);

    texOpt = boingGetLeafTexturesOption (d);
    sd->boingTexFiles = texOpt->value.list.value;
    sd->boingTexNFiles = texOpt->value.list.nValue;

    d->base.privates[displayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
boingFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    BOING_DISPLAY (d);

    freeScreenPrivateIndex (d, sd->screenPrivateIndex);
    free (sd);
}

static Bool
boingInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
boingFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool boingInitObject(CompPlugin *p, CompObject *o){

    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, // InitCore
	(InitPluginObjectProc) boingInitDisplay,
	(InitPluginObjectProc) boingInitScreen
    };

    RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void boingFiniObject(CompPlugin *p, CompObject *o){

    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, // FiniCore
	(FiniPluginObjectProc) boingFiniDisplay,
	(FiniPluginObjectProc) boingFiniScreen
    };

    DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}

CompPluginVTable boingVTable = {
    "boing",
    0,
    boingInit,
    boingFini,
    boingInitObject,
    boingFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &boingVTable;
}
