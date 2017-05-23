/**
 *
 * Compiz Roach plugin
 *
 * roach.c
 * 
 * Roach was adapted from the Snow plugin by Patrick Fisher. The copywrites and warranty 
 * from the Snow plugin remain in effect, as shown below.
 *
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 * Snow plugin maintained by Danny Baumann <maniac@opencompositing.org>
 * Roach Plugin maintained by Patrick Fisher <roachplugin@gmail.com>
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


#include <math.h>

#include <compiz-core.h>
#include "roach_options.h"
#include <time.h>


#define GET_ROACH_DISPLAY(d)                            \
    ((RoachDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define ROACH_DISPLAY(d)                                \
    RoachDisplay *sd = GET_ROACH_DISPLAY (d)

#define GET_ROACH_SCREEN(s, sd)                         \
    ((LeafScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)

#define ROACH_SCREEN(s)                                 \
    LeafScreen *ss = GET_ROACH_SCREEN (s, GET_ROACH_DISPLAY (s->display))

static int displayPrivateIndex = 0;

/* -------------------  STRUCTS ----------------------------- */
typedef struct _RoachDisplay
{
    int screenPrivateIndex;

    Bool useTextures;

    int             roachTexNFiles;
    CompOptionValue *roachTexFiles;
} RoachDisplay;

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
    float spin;
    float spinspeed;
    int roach_direction;
    float roach_speed;
    int frame_flag;
    int frame_flag_counter;


    LeafTexture *tex;
} Leaf;

typedef struct _LeafScreen
{
    CompScreen *s;

    Bool active;

    CompTimeoutHandle timeoutHandle;

    PaintOutputProc paintOutput;
    DrawWindowProc  drawWindow;

    LeafTexture *roachTex;
    int         roachTexturesLoaded;

    GLuint displayList;
    Bool   displayListNeedsUpdate;

    Leaf *allLeaves;
} LeafScreen;

/* some forward declarations */
static void initiateLeaf (LeafScreen * ss, Leaf * sf);

static void
roachThink (LeafScreen *ss,
	   Leaf  *sf)
{
	float roach_change;
	float roach_move_decide;
	int moveornot;

moveornot=0;
//move?
roach_move_decide = rand() % (100 - 1 + 1) + 1;
if (roach_move_decide > 50)
{
moveornot=1;
//change direction?
roach_change = rand() % (100 - 1 + 1) + 1;
if (roach_change >80)
	//direction to face
	sf->roach_direction = rand() % (4 - 1 + 1) + 1;
}


if (moveornot==1)
{
//up
if (sf->roach_direction == 1)
	{
	sf->frame=3;
	if (sf->frame_flag==1)
		sf->frame=7;
	sf->x=sf->x-sf->roach_speed;
    if (sf->x <= 1)
	{
	sf->x=2;
	sf->roach_direction=rand() % (4 - 1 + 1) + 1;
	}
	}
	

//right
if (sf->roach_direction == 2)
	{
	sf->frame=1;
	if (sf->frame_flag==1)
		sf->frame=5;
	sf->x=sf->x+sf->roach_speed;
	if (sf->x >= ss->s->width)
	{
	sf->x=ss->s->width-2;
	sf->roach_direction = rand() % (4 - 1 + 1) + 1;
	}
	}

//down
if (sf->roach_direction == 3)
	{
	sf->frame=2;
	if (sf->frame_flag==1)
		sf->frame=6;
	sf->y=sf->y+sf->roach_speed;
	if (sf->y >= ss->s->height)
	{
	sf->y = ss->s->height-2;
	sf->roach_direction = rand() % (4 - 1 + 1) + 1;
	}
	}

//left
if (sf->roach_direction == 4)
	{
	sf->frame=0;
	if (sf->frame_flag==1)
		sf->frame=4;
	sf->y=sf->y-sf->roach_speed;
    if (sf->y <= 1)
	{
	sf->y=2;
	sf->roach_direction = rand() % (4 - 1 + 1) + 1;
	}
	}
}



    sf->frame_flag=sf->frame_flag*-1;

    sf->tex = &ss->roachTex[sf->frame];
}

static Bool
stepLeafPositions (void *closure)
{
    CompScreen *s = closure;
    int        i, numLeaves;
    Leaf  *roachLeaf;
    Bool       onTop;

    ROACH_SCREEN (s);

    if (!ss->active)
	return TRUE;

    roachLeaf = ss->allLeaves;
    numLeaves = roachGetNumLeaves (s->display);
    onTop = roachGetLeavesOverWindows (s->display);

    for (i = 0; i < numLeaves; i++)
	roachThink(ss, roachLeaf++);

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
roachToggle (CompDisplay     *d,
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
	ROACH_SCREEN (s);
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
    float roachSize = roachGetLeafSize (ss->s->display);

    ss->displayList = glGenLists (1);

    glNewList (ss->displayList, GL_COMPILE);
    glBegin (GL_QUADS);

    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, 0, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, roachSize, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (roachSize, roachSize, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (roachSize, 0, -0.0);

    glEnd ();
    glEndList ();
}

static void
beginRendering (LeafScreen *ss,
		CompScreen *s)
{

    if (roachGetUseBlending (s->display))
	glEnable (GL_BLEND);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (ss->displayListNeedsUpdate)
    {
	setupDisplayList (ss);
	ss->displayListNeedsUpdate = FALSE;
    }

    glColor4f (1.0, 1.0, 1.0, 1.0);
    if (ss->roachTexturesLoaded && roachGetUseTextures (s->display))
    {
	int j;

	for (j = 0; j < ss->roachTexturesLoaded; j++)
	{
	    Leaf *roachLeaf = ss->allLeaves;
	    int       i, numLeaves = roachGetNumLeaves (s->display);
	    Bool      roachRotate = roachGetLeafRotation (s->display);

	    enableTexture (ss->s, &ss->roachTex[j].tex,
			   COMP_TEXTURE_FILTER_GOOD);

	    for (i = 0; i < numLeaves; i += 2)
	    {
		if (roachLeaf->tex == &ss->roachTex[j])
		{
		    glTranslatef (roachLeaf->x, roachLeaf->y, roachLeaf->z);
		
	    	    if (roachRotate)
			glRotatef (roachLeaf->ra, 0, 0, 1);
	    	    glCallList (ss->roachTex[j].dList);
    		    if (roachRotate)
			glRotatef (-roachLeaf->ra, 0, 0, 1);
	    	    glTranslatef (-roachLeaf->x, -roachLeaf->y, -roachLeaf->z);
		
		}
		roachLeaf++;
	    }

	    for (i = 1; i < (numLeaves); i += 2)
	    {
		if (roachLeaf->tex == &ss->roachTex[j])
		{
		    glTranslatef (roachLeaf->x, roachLeaf->y, roachLeaf->z);
		
	    	    if (roachRotate)
			glRotatef (-roachLeaf->ra, 0, 0, 1);
	    	    glCallList (ss->roachTex[j].dList);
    		    if (roachRotate)
			glRotatef (roachLeaf->ra, 0, 0, 1);
	    	    glTranslatef (-roachLeaf->x, -roachLeaf->y, -roachLeaf->z);
		
		}
		roachLeaf++;
	    }
	    disableTexture (ss->s, &ss->roachTex[j].tex);
	}
    }
    else
    {
	Leaf *roachLeaf = ss->allLeaves;
	int       i, numLeaves = roachGetNumLeaves (s->display);

	for (i = 0; i < numLeaves; i++)
	{
	    glTranslatef (roachLeaf->x, roachLeaf->y, roachLeaf->z);
	    glRotatef (roachLeaf->ra, 0, 0, 1);
	    glCallList (ss->displayList);
	    glRotatef (-roachLeaf->ra, 0, 0, 1);
	    glTranslatef (-roachLeaf->x, -roachLeaf->y, -roachLeaf->z);
	    roachLeaf++;
	}
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    if (roachGetUseBlending (s->display))
    {
	glDisable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

/* ------------------------  FUNCTIONS -------------------- */

static Bool
roachPaintOutput (CompScreen              *s,
		 const ScreenPaintAttrib *sa,
		 const CompTransform	 *transform,
		 Region                  region,
		 CompOutput              *output, 
		 unsigned int            mask)
{
    Bool status;

    ROACH_SCREEN (s);

    if (ss->active && !roachGetLeavesOverWindows (s->display))
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (ss, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ss, s, paintOutput, roachPaintOutput);

    if (ss->active && roachGetLeavesOverWindows (s->display))
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
roachDrawWindow (CompWindow           *w,
		const CompTransform  *transform,
		const FragmentAttrib *attrib,
		Region               region,
		unsigned int         mask)
{
    Bool status;

    ROACH_SCREEN (w->screen);

    /* First draw Window as usual */
    UNWRAP (ss, w->screen, drawWindow);
    status = (*w->screen->drawWindow) (w, transform, attrib, region, mask);
    WRAP (ss, w->screen, drawWindow, roachDrawWindow);

    /* Check whether this is the Desktop Window */
    if (ss->active && (w->type & CompWindowTypeDesktopMask) && 
	!roachGetLeavesOverWindows (w->screen->display))
    {
	beginRendering (ss, w->screen);
    }

    return status;
}

static void
initiateLeaf (LeafScreen *ss,
		   Leaf  *sf)
{
    int boxing = roachGetScreenBoxing (ss->s->display);

    switch (roachGetWindDirection (ss->s->display))
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

    sf->z  = mmRand (-roachGetScreenDepth (ss->s->display), 0.1, 5000);
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
    sf->roach_direction = getRand (1,4);
    sf->roach_speed = getRand(4,8);
	sf->frame_flag=-1;
}

static void
setLeafTexture (LeafScreen *ss,
		     Leaf  *sf)
{
    if (ss->roachTexturesLoaded)
	sf->tex = &ss->roachTex[rand () % ss->roachTexturesLoaded];
}

static void
updateLeafTextures (CompScreen *s)
{
    int       i, count = 0;
    float     roachSize = roachGetLeafSize(s->display);
    int       numLeaves = roachGetNumLeaves(s->display);
    Leaf *roachLeaf;

    ROACH_SCREEN (s);
    ROACH_DISPLAY (s->display);

    roachLeaf = ss->allLeaves;

    for (i = 0; i < ss->roachTexturesLoaded; i++)
    {
	finiTexture (s, &ss->roachTex[i].tex);
	glDeleteLists (ss->roachTex[i].dList, 1);
    }

    if (ss->roachTex)
	free (ss->roachTex);
    ss->roachTexturesLoaded = 0;

    ss->roachTex = calloc (1, sizeof (LeafTexture) * sd->roachTexNFiles);

    for (i = 0; i < sd->roachTexNFiles; i++)
    {
	CompMatrix  *mat;
	LeafTexture *sTex;

	ss->roachTex[count].loaded =
	    readImageToTexture (s, &ss->roachTex[count].tex,
				sd->roachTexFiles[i].s,
				&ss->roachTex[count].width,
				&ss->roachTex[count].height);
	if (!ss->roachTex[count].loaded)
	{
	    compLogMessage ("roach", CompLogLevelWarn,
			    "Texture not found : %s", sd->roachTexFiles[i].s);
	    continue;
	}
	compLogMessage ("roach", CompLogLevelInfo,
			"Loaded Texture %s", sd->roachTexFiles[i].s);
	
	mat = &ss->roachTex[count].tex.matrix;
	sTex = &ss->roachTex[count];

	sTex->dList = glGenLists (1);
	glNewList (sTex->dList, GL_COMPILE);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
	glVertex2f (0, 0);
	glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, sTex->height));
	glVertex2f (0, roachSize * sTex->height / sTex->width);
	glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->width),
		      COMP_TEX_COORD_Y (mat, sTex->height));
	glVertex2f (roachSize, roachSize * sTex->height / sTex->width);
	glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->width),
		      COMP_TEX_COORD_Y (mat, 0));
	glVertex2f (roachSize, 0);

	glEnd ();
	glEndList ();

	count++;
    }

    ss->roachTexturesLoaded = count;
    if (count < sd->roachTexNFiles)
	ss->roachTex = realloc (ss->roachTex, sizeof (LeafTexture) * count);

    for (i = 0; i < numLeaves; i++)
	setLeafTexture (ss, roachLeaf++);
}

static Bool
roachInitScreen (CompPlugin *p,
		CompScreen *s)
{
    LeafScreen *ss;
    int        i, numLeaves = roachGetNumLeaves (s->display);
    Leaf  *roachLeaf;

    ROACH_DISPLAY (s->display);

    ss = calloc (1, sizeof(LeafScreen));

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    ss->s = s;
    ss->roachTexturesLoaded = 0;
    ss->roachTex = NULL;
    ss->active = FALSE;
    ss->displayListNeedsUpdate = FALSE;

    ss->allLeaves = roachLeaf = malloc (numLeaves * sizeof (Leaf));

    for (i = 0; i < numLeaves; i++)
    {
	initiateLeaf (ss, roachLeaf);
	setLeafTexture (ss, roachLeaf);
	roachLeaf++;
    }

    updateLeafTextures (s);
    setupDisplayList (ss);

    WRAP (ss, s, paintOutput, roachPaintOutput);
    WRAP (ss, s, drawWindow, roachDrawWindow);

    ss->timeoutHandle = compAddTimeout (roachGetAutumnUpdateDelay (s->display),
					roachGetAutumnUpdateDelay (s->display),
					stepLeafPositions, s);

    return TRUE;
}

static void
roachFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    int i;

    ROACH_SCREEN (s);

    if (ss->timeoutHandle)
	compRemoveTimeout (ss->timeoutHandle);

    for (i = 0; i < ss->roachTexturesLoaded; i++)
    {
	finiTexture (s, &ss->roachTex[i].tex);
	glDeleteLists (ss->roachTex[i].dList, 1);
    }

    if (ss->roachTex)
	free (ss->roachTex);

    if (ss->allLeaves)
	free (ss->allLeaves);

    UNWRAP (ss, s, paintOutput);
    UNWRAP (ss, s, drawWindow);

    free (ss);
}

static void
roachDisplayOptionChanged (CompDisplay        *d,
			  CompOption         *opt,
			  RoachDisplayOptions num)
{
    ROACH_DISPLAY (d);

    switch (num)
    {
    case RoachDisplayOptionLeafSize:
	{
	    CompScreen *s;

	    for (s = d->screens; s; s = s->next)
	    {
		ROACH_SCREEN (s);
		ss->displayListNeedsUpdate = TRUE;
		updateLeafTextures (s);
	    }
	}
	break;
    case RoachDisplayOptionAutumnUpdateDelay:
	{
	    CompScreen *s;

	    for (s = d->screens; s; s = s->next)
	    {
		ROACH_SCREEN (s);
					
		if (ss->timeoutHandle)
		    compRemoveTimeout (ss->timeoutHandle);
		ss->timeoutHandle =
		    compAddTimeout (roachGetAutumnUpdateDelay (d),
				    roachGetAutumnUpdateDelay (d),
				    stepLeafPositions, s);
	    }
	}
	break;
    case RoachDisplayOptionNumLeaves:
	{
	    CompScreen *s;
	    int        i, numLeaves;
	    Leaf  *roachLeaf;

	    numLeaves = roachGetNumLeaves (d);
	    for (s = d->screens; s; s = s->next)
    	    {
		ROACH_SCREEN (s);
		ss->allLeaves = realloc (ss->allLeaves,
					     numLeaves * sizeof (Leaf));
		roachLeaf = ss->allLeaves;

		for (i = 0; i < numLeaves; i++)
		{
		    initiateLeaf (ss, roachLeaf);
    		    setLeafTexture (ss, roachLeaf);
	    	    roachLeaf++;
		}
	    }
	}
	break;
    case RoachDisplayOptionLeafTextures:
	{
	    CompScreen *s;
	    CompOption *texOpt;

	    texOpt = roachGetLeafTexturesOption (d);

	    sd->roachTexFiles = texOpt->value.list.value;
    	    sd->roachTexNFiles = texOpt->value.list.nValue;

	    for (s = d->screens; s; s = s->next)
		updateLeafTextures (s);
	}
	break;
    case RoachDisplayOptionDefaultEnabled:
	{
	    CompScreen *s;
	    for (s = d->screens; s; s = s->next)
	    {
		ROACH_SCREEN (s);
		ss->active = roachGetDefaultEnabled(s->display);
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
roachInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    CompOption  *texOpt;
    RoachDisplay *sd;
    
    sd = malloc (sizeof (RoachDisplay));

    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (sd->screenPrivateIndex < 0)
    {
	free (sd);
	return FALSE;
    }
	
    roachSetToggleKeyInitiate (d, roachToggle);
    roachSetNumLeavesNotify (d, roachDisplayOptionChanged);
    roachSetLeafSizeNotify (d, roachDisplayOptionChanged);
    roachSetAutumnUpdateDelayNotify (d, roachDisplayOptionChanged);
    roachSetLeafTexturesNotify (d, roachDisplayOptionChanged);

    texOpt = roachGetLeafTexturesOption (d);
    sd->roachTexFiles = texOpt->value.list.value;
    sd->roachTexNFiles = texOpt->value.list.nValue;

    d->base.privates[displayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
roachFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    ROACH_DISPLAY (d);

    freeScreenPrivateIndex (d, sd->screenPrivateIndex);
    free (sd);
}

static Bool
roachInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
roachFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool roachInitObject(CompPlugin *p, CompObject *o){

    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, // InitCore
	(InitPluginObjectProc) roachInitDisplay,
	(InitPluginObjectProc) roachInitScreen
    };

    RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void roachFiniObject(CompPlugin *p, CompObject *o){

    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, // FiniCore
	(FiniPluginObjectProc) roachFiniDisplay,
	(FiniPluginObjectProc) roachFiniScreen
    };

    DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}

CompPluginVTable roachVTable = {
    "roach",
    0,
    roachInit,
    roachFini,
    roachInitObject,
    roachFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &roachVTable;
}
