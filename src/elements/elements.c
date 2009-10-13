/**
 * Compiz elements plugin
 * elements.c
 *
 * This plugin allows you to draw different 'elements' on your screen
 * such as snow, fireflies, starts, leaves and bubbles. It also has
 * a pluggable element creation interface
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 * Copyright (c) 2008 Patrick Fisher <pat@elementsplugin.com>
 *
 * This plugin was based on the works of the following authors:
 *
 * Snow Plugin:
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 *
 * Fireflies Plugin:
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 *
 * Stars Plugin:
 * Copyright (c) 2007 Kyle Mallory <kyle.mallory@utah.edu>
 *
 * Autumn Plugin
 * Copyright (c) 2007 Patrick Fisher <pat@elementsplugin.com>
 *
 * Extensions interface largely based off the Animation plugin
 * Copyright (c) 2006 Erkin Bahceci <erkinbah@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 **/

#include <compiz-core.h>
#include <string.h>
#include <math.h>
#include "elements-internal.h"

int displayPrivateIndex;
int functionsPrivateIndex;

int
elementsGetRand (int min,
		 int max)
{
    return (rand () % (max - min + 1) + min);
}

float
elementsMmRand (int   min,
		int   max,
		float divisor)
{
    return ((float) elementsGetRand (min, max)) / divisor;
}

int elementsGetEBoxing (CompScreen *s)
{
    return elementsGetScreenBoxing (s);
}

int elementsGetEDepth (CompScreen *s)
{
    return elementsGetScreenDepth (s);
}

/* Check to see if an element is outside the screen. If it is
 * remove it and re-initiate it. Otherwise move it */

static void
elementTestCreate (CompScreen       *s,
		   Element          *ele,
		   ElementAnimation *anim)
{
    if (ele)
    {
	if ((ele->y >= s->height + 200                               ||
	     ele->x <= -200                                          ||
	     ele->x >= s->width + 200                                ||
	     ele->y >= s->height + 200                               ||
	     ele->z <= -((float) elementsGetScreenDepth (s) / 500.0) ||
	     ele->z >= ((float)elementsGetScreenBoxing (s) / 5.0f)))
	{
	    if (anim->properties->fini)
		(*anim->properties->fini) (s, ele);
	    initiateElement(s, anim, ele);
	}
    }

    elementMove(s, ele, anim, elementsGetUpdateDelay (s));
}

/* Check to see if an element is still within the screen. If it is,
 * return FALSE and move all elements. If every element isn't,
 * return TRUE
 */

static Bool
elementTestOffscreen (CompScreen       *s,
		      ElementAnimation *anim)
{
    int     i;
    Element *e;
    Bool    ret = TRUE;

    for (i = 0; i < anim->nElement; i++)
    {
	e = &anim->elements[i];
	if (e)
	{
	    if (!(e->y >= s->height + 200                               ||
		  e->x <= -200                                          ||
		  e->x >= s->width + 200                                ||
		  e->y >= s->height + 200                               ||
		  e->z <= -((float) elementsGetScreenDepth (s) / 500.0) ||
		  e->z >= 1                                             ||
		  e->y <= -((float) elementsGetScreenBoxing (s) / 5.0f)) &&
		e->opacity > 0.0f)
	    {
		ret = FALSE;
	    }
	    else
	    {
		if (anim->properties->fini)
		    (*anim->properties->fini) (s, e);
	    }
	}
	if (e->opacity > 0.0f)
	    e->opacity -= 0.003f;
    }

    if (!ret)
    {
	for (i = 0; i < anim->nElement; i++)
	{
	    elementMove (s, &anim->elements[i], anim,
			 elementsGetUpdateDelay (s));
	}
    }

    return ret;
}

/* Element Text Display Management */

static void
elementsFreeTitle (CompScreen *s)
{
    ELEMENTS_SCREEN (s);
    ELEMENTS_DISPLAY (s->display);

    if (!es->textData)
	return;

    (ed->textFunc->finiTextData) (s, es->textData);
    es->textData = NULL;

    damageScreen (s);
}

static void
elementsRenderTitle (CompScreen *s,
		     char       *stringData)
{
    CompTextAttrib tA;
    int            ox1, ox2, oy1, oy2;

    ELEMENTS_SCREEN (s);
    ELEMENTS_DISPLAY (s->display);

    elementsFreeTitle (s);

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    /* 75% of the output device es maximum width */
    tA.maxWidth = (ox2 - ox1) * 3 / 4;
    tA.maxHeight = 100;

    tA.family = "Sans";
    tA.size = elementsGetTitleFontSize (s);
    tA.color[0] = elementsGetTitleFontColorRed (s);
    tA.color[1] = elementsGetTitleFontColorGreen (s);
    tA.color[2] = elementsGetTitleFontColorBlue (s);
    tA.color[3] = elementsGetTitleFontColorAlpha (s);

    tA.flags = CompTextFlagWithBackground | CompTextFlagEllipsized;

    tA.bgHMargin = 10.0f;
    tA.bgVMargin = 10.0f;
    tA.bgColor[0] = elementsGetTitleBackColorRed (s);
    tA.bgColor[1] = elementsGetTitleBackColorGreen (s);
    tA.bgColor[2] = elementsGetTitleBackColorBlue (s);
    tA.bgColor[3] = elementsGetTitleBackColorAlpha (s);

    es->textData = (ed->textFunc->renderText) (s, stringData, &tA);
}

/* Taken from ring.c */

static void
elementsDrawTitle (CompScreen *s)
{
    GLboolean  wasBlend;
    GLint      oldBlendSrc, oldBlendDst;
    float      x, y, width, height;
    int        ox1, ox2, oy1, oy2, k;
    CompMatrix *m;

    ELEMENTS_SCREEN(s);

    width  = es->textData->width;
    height = es->textData->height;

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    x = floor (ox1 + ((ox2 - ox1) / 2) - (width / 2));
    y = floor (oy1 + ((oy2 - oy1) * 3 / 4) + (height / 2));

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);
    wasBlend = glIsEnabled (GL_BLEND);

    if (!wasBlend)
	glEnable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);

    enableTexture (s, es->textData->texture, COMP_TEXTURE_FILTER_GOOD);

    m = &es->textData->texture->matrix;

    if (es->renderTexture && es->eTexture)
    {
	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
	glVertex2f (x - es->eTexture[es->nTexture].width - 20, y - height);
	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x - es->eTexture[es->nTexture].width - 20, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width),
		      COMP_TEX_COORD_Y (m, height));
	glVertex2f (x - es->eTexture[es->nTexture].width - 20 + width, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (x - es->eTexture[es->nTexture].width - 20 + width,
		    y - height);

	glEnd ();
    }
    else
    {
	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
	glVertex2f (x, y - height);
	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x + width, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (x + width, y - height);

	glEnd ();
    }
    disableTexture (s, es->textData->texture);

    if (es->renderTexture && es->eTexture)
    {
	int border = 5;
	width  = es->eTexture[es->nTexture].width;
	height = es->eTexture[es->nTexture].height;

	glPushMatrix ();

	glColor4f ((float) elementsGetTitleBackColorRed (s) / OPAQUE,
		   (float) elementsGetTitleBackColorGreen (s) / OPAQUE,
		   (float) elementsGetTitleBackColorBlue (s) / OPAQUE,
		   (float) elementsGetTitleBackColorAlpha (s) / OPAQUE);

	glTranslatef (x + es->textData->width - width,
		      y - height + (border / 2),
		      0.0f);
	glRectf (0.0f, height, width, 0.0f);
	glRectf (0.0f, 0.0f, width, -border);
	glRectf (0.0f, height + border, width, height);
	glRectf (-border, height, 0.0f, 0.0f);
	glRectf (width, height, width + border, 0.0f);
	glTranslatef (-border, -border, 0.0f);

#define CORNER(a,b) \
for (k = a; k < b; k++) \
{\
float rad = k * (M_PI / 180.0f);\
glVertex2f (0.0f, 0.0f);\
glVertex2f (cos (rad) * border, sin (rad) * border);\
glVertex2f (cos ((k - 1) * (M_PI / 180.0f)) * border, \
	    sin ((k - 1) * (M_PI / 180.0f)) * border);\
}

	glTranslatef (border, border, 0.0f);
	glBegin (GL_TRIANGLES);
	CORNER (180, 270) glEnd ();
	glTranslatef (-border, -border, 0.0f);

	glTranslatef (width + border, border, 0.0f);
	glBegin (GL_TRIANGLES);
	CORNER (270, 360) glEnd ();
	glTranslatef (-(width + border), -border, 0.0f);

	glTranslatef (border, height + border, 0.0f);
	glBegin (GL_TRIANGLES);
	CORNER (90, 180) glEnd ();
	glTranslatef (-border, -(height + border), 0.0f);

	glTranslatef (width + border, height + border, 0.0f);
	glBegin (GL_TRIANGLES);
	CORNER (0, 90) glEnd ();
	glTranslatef (-(width + border), -(height + border), 0.0f);

	glColor4usv(defaultColor);

	enableTexture (s, &es->eTexture[es->nTexture].tex,
		       COMP_TEXTURE_FILTER_GOOD);
	glTranslatef (border, border, 0.0f);
	glCallList (es->eTexture[es->nTexture].dList);
	disableTexture (s, &es->eTexture[es->nTexture].tex);

	glPopMatrix ();

#undef CORNER
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

/* Element Animation Management */

static Bool
elementsPropertiesForAnimation (CompDisplay      *d,
				ElementAnimation *anim,
				char             *name)
{
    ELEMENTS_DISPLAY (d);

    ElementTypeInfo *info;

    for (info = ed->elementTypes; info; info = info->next)
    {
	if (!strcmp (info->name, name))
	{
	    anim->properties = info;
	    return TRUE;
	}
    }

    return FALSE;
}

ElementAnimation *
elementsCreateAnimation (CompScreen *s,
			 char       *name)
{
    ElementAnimation *anim;

    ELEMENTS_SCREEN (s);

    if (!es->animations)
    {
	es->animations = calloc (1, sizeof (ElementAnimation));
	if (!es->animations)
	    return NULL;

	es->animations->next = NULL;
	anim = es->animations;
    }
    else
    {
	/* Exhaust the list, last entry */
	for (anim = es->animations; anim->next; anim = anim->next) ;

	anim->next = calloc (1, sizeof (ElementAnimation));
	if (!anim->next)
	    return NULL;

	anim->next->next = NULL;
	anim = anim->next;
    }

    if (!elementsPropertiesForAnimation (s->display, anim, name))
    {
	compLogMessage ("elements", CompLogLevelWarn,
			"Could not find element movement pattern %s, "
			"disabling this element", name);
	free (anim);
	return NULL;
    }

    return anim;
}

void
elementsDeleteAnimation (CompScreen       *s,
			 ElementAnimation *anim)
{
    ElementAnimation *run;

    ELEMENTS_SCREEN (s);

    if (es->animations)
    {
	if (anim == es->animations)
	{
	    if (es->animations->next)
		es->animations = es->animations->next;
	    else
		es->animations = NULL;

	    free (anim);
	}
	for (run = es->animations; run; run = run->next)
	{
	    if (anim == run->next)
	    {
		if (run->next->next)
		    run->next = run->next->next;
	        else
		    run->next = NULL;

	    	free (anim);
		break;
	    }
    	}
    }
}

/* Type Info Management */

static Bool
elementsCreateNewElementType (CompDisplay         *d,
			      char                *name,
			      char                *desc,
			      ElementInitiateProc initiate,
			      ElementMoveProc     move,
			      ElementFiniProc     fini)
{
    ElementTypeInfo *info;

    ELEMENTS_DISPLAY (d);

    if (!ed->elementTypes)
    {
	ed->elementTypes = calloc (1, sizeof (ElementTypeInfo));
	if (!ed->elementTypes)
	    return FALSE;

	ed->elementTypes->next = NULL;
	info = ed->elementTypes;
    }
    else
    {
	/* Exhaust the list, last entry */
	for (info = ed->elementTypes; info->next; info = info->next) ;

	info->next = calloc (1, sizeof (ElementTypeInfo));
	if (!info->next)
	    return FALSE;

	info->next->next = NULL;
	info = info->next;
    }

    info->name     = name;
    info->desc     = desc;
    info->initiate = initiate;
    info->move     = move;
    info->fini     = fini;

    return TRUE;
}

static void
elementsRemoveElementType (CompScreen *s,
			   char       *name)
{
    ElementAnimation *anim, *next;
    ElementTypeInfo  *info;

    ELEMENTS_DISPLAY (s->display);
    ELEMENTS_SCREEN (s);

    for (anim = es->animations; anim; anim = next)
    {
	next = anim->next;

	if (!strcmp (anim->type, name))
	{
	    int i;

	    for (i = 0; i < anim->nTextures; i++)
	    {
	        finiTexture (s, &anim->texture[i].tex);
		glDeleteLists (anim->texture[i].dList, 1);
	    }

	    for (i = 0; i < anim->nElement; i++)
	    {
	        if (anim->properties->fini)
	            (*anim->properties->fini) (s, &anim->elements[i]);
	    }

	    free (anim->elements);
	    free (anim->texture);
	    free (anim->type);
	    elementsDeleteAnimation (s, anim);
	}
    }

    for (info = ed->elementTypes; info; info = info->next)
	if (!strcmp (info->name, name))
	    break;

    if (info)
    {
	if (info == ed->elementTypes)
	{
	    if (ed->elementTypes->next)
		ed->elementTypes = ed->elementTypes->next;
	    else
		ed->elementTypes = NULL;

	    free (info);
	}
	else
	{
	    ElementTypeInfo *run;

	    for (run = ed->elementTypes; run; run = run->next)
	    {
		if (run->next == info)
		{
		    if (run->next)
		        run->next = run->next->next;
		    else
			run->next = NULL;

		    free (info);
		    break;
		}
	    }
	}
    }
}

static Bool
textureToAnimation (CompScreen       *s,
		    ElementAnimation *anim,
		    CompListValue    *paths,
		    CompListValue    *iters,
		    int              size,
		    int              iter)
{
    ElementTexture *aTex;
    CompMatrix	   *mat;
    int            i;
    int		   texIter = 0;

    for (i = 0; i < iters->nValue; i++)
	if (iters->value[i].i == iter)
	    anim->nTextures++;

    anim->texture = realloc (anim->texture,
			     sizeof (ElementTexture) * anim->nTextures);
    if (!anim->texture)
	return FALSE;

    for (i = 0; i < iters->nValue; i++)
    {
	if (iters->value[i].i == iter)
	{
	    if (paths->value[i].s)
	    {
		initTexture (s, &anim->texture[texIter].tex);
		anim->texture[texIter].loaded =
		    readImageToTexture (s,
					&anim->texture[texIter].tex,
					paths->value[i].s,
					&anim->texture[texIter].width,
					&anim->texture[texIter].height);

		if (!anim->texture[texIter].loaded)
		{
		    compLogMessage ("elements", CompLogLevelWarn,
				    "Texture for animation %s not found at"
				    " location %s or invalid",
				     anim->type, paths->value[i].s);
		    return FALSE;
		}
		else
		{
		    compLogMessage ("elements", CompLogLevelInfo,
				    "Loaded Texture %s for animation %s",
				     paths->value[i].s, anim->type);
		}

		mat = &anim->texture[texIter].tex.matrix;
		aTex = &anim->texture[texIter];
		aTex->dList = glGenLists (1);
		glNewList (aTex->dList, GL_COMPILE);

		glBegin (GL_QUADS);

		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			      COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (0, 0);
		glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			  COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (0, size * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
			  COMP_TEX_COORD_Y (mat, aTex->height));
		glVertex2f (size, size * aTex->height / aTex->width);
		glTexCoord2f (COMP_TEX_COORD_X (mat, aTex->width),
			  COMP_TEX_COORD_Y (mat, 0));
		glVertex2f (size, 0);

		glEnd ();
		glEndList ();

		texIter++;
	    }
	}
    }

    return TRUE;
}

static Bool
createElementAnimation (CompScreen *s,
			int        nElement,
			char       *type,
			int        size,
			int        speed,
			int        iter)
{
    ElementAnimation *anim = elementsCreateAnimation (s, type);
    CompListValue    *paths, *iters;

    if (!anim)
	return FALSE;

    paths = elementsGetElementImage (s);
    iters = elementsGetElementIter  (s);

    anim->nElement  = nElement;
    anim->size      = size;
    anim->speed     = speed;
    anim->id        = iter;
    anim->type      = strdup (type);
    anim->nTextures = 0;

    if (textureToAnimation (s, anim, paths, iters, size, iter) &&
	anim->nTextures)
    {
	Element *e;

	anim->elements = realloc (anim->elements,
				  sizeof (Element) * nElement);
	/* FIXME: NULL check? */

	e = anim->elements;
	while (nElement--)
	{
	    initiateElement (s, anim, e);
	    e++;
	}
	anim->active = TRUE;

	return TRUE;
    }

    if (anim->texture)
	free (anim->texture);
    elementsDeleteAnimation (s, anim);

    return FALSE;
}

/* Timeout callbacks */

static Bool
elementsRemoveTimeout (void *closure)
{
    CompScreen *s = (CompScreen *) closure;
    int        i;

    ELEMENTS_SCREEN (s);

    es->renderText    = FALSE;
    es->renderTexture = FALSE;

    elementsFreeTitle (s);

    for (i = 0; i < es->ntTextures; i++)
    {
	finiTexture (s, &es->eTexture[i].tex);
	glDeleteLists (es->eTexture[i].dList, 1);
    }

    free (es->eTexture);
    es->eTexture = NULL;

    damageScreen (s);

    if (es->switchTimeout)
	compRemoveTimeout (es->switchTimeout);

    return FALSE;
}

static Bool
elementsSwitchTextures (void *closure)
{
    CompScreen *s = (CompScreen *) closure;

    ELEMENTS_SCREEN (s);

    es->nTexture = ++es->nTexture % (es->ntTextures);
    damageScreen (s);

    return TRUE;
}

static void
addDisplayTimeouts (CompScreen *s,
		    Bool       switchIt)
{
    int time = elementsGetTitleDisplayTime (s);

    ELEMENTS_SCREEN (s);

    if (es->renderTimeout)
	compRemoveTimeout (es->renderTimeout);

    es->renderTimeout = compAddTimeout (time, time * 2.0,
					elementsRemoveTimeout, s);

    if (switchIt)
    {
	if (es->switchTimeout)
	    compRemoveTimeout (es->switchTimeout);

	es->switchTimeout = compAddTimeout (time / es->ntTextures,
					    time * 2 / es->ntTextures,
					    elementsSwitchTextures, s);
    }
}

static Bool
createTemporaryTexture (CompScreen    *s,
		        CompListValue *paths,
			CompListValue *iters,
			int           iter,
		        int           size)
{
    CompMatrix *mat;
    int        i;
    int	       texIter = 0;

    ELEMENTS_SCREEN (s);

    es->ntTextures = 0;
    es->nTexture   = 0;

    if (es->eTexture)
    {
	for (i = 0; i < es->ntTextures; i++)
	{
	    finiTexture (s, &es->eTexture[i].tex);
	    glDeleteLists (es->eTexture[i].dList, 1);
	}
    }

    for (i = 0; i < iters->nValue; i++)
    {
	if (iters->value[i].i == iter)
	    es->ntTextures++;
    }
    es->eTexture = realloc (es->eTexture,
			    sizeof (ElementTexture) * es->ntTextures);

    if (!es->eTexture)
	return FALSE;

    for (i = 0; i < iters->nValue; i++)
    {
	if (iters->value[i].i == iter)
	{
	    initTexture (s, &es->eTexture[texIter].tex);

            es->eTexture[texIter].loaded =
		readImageToTexture (s,
				    &es->eTexture[texIter].tex,
				    paths->value[i].s,
				    &es->eTexture[texIter].width,
				    &es->eTexture[texIter].height);
	    if (!es->eTexture[texIter].loaded)
	    {
		compLogMessage ("elements", CompLogLevelWarn,
			        "Texture not found or invalid at %s",
			         paths->value[i].s);
		return FALSE;
	    }
	    else
	    {
		compLogMessage ("elements", CompLogLevelInfo,
				"Loaded Texture %s", paths->value[i].s);
	    }

	    mat = &es->eTexture[texIter].tex.matrix;
	    es->eTexture[texIter].dList = glGenLists (1);
	    glNewList (es->eTexture[texIter].dList, GL_COMPILE);

	    glBegin (GL_QUADS);

	    glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			  COMP_TEX_COORD_Y (mat, 0));
	    glVertex2f (0, 0);
	    glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
			  COMP_TEX_COORD_Y (mat,
					    es->eTexture[texIter].height));
	    glVertex2f (0, size);
	    glTexCoord2f (COMP_TEX_COORD_X (mat, es->eTexture[texIter].width),
			  COMP_TEX_COORD_Y (mat,
					    es->eTexture[texIter].height));
	    glVertex2f (size, size);
	    glTexCoord2f (COMP_TEX_COORD_X (mat, es->eTexture[texIter].width),
			  COMP_TEX_COORD_Y (mat, 0));
	    glVertex2f (size, 0);

	    es->eTexture[texIter].height = size;
	    es->eTexture[texIter].width = size;

	    glEnd ();
	    glEndList ();

	    texIter++;
	}
    }

    return TRUE;
}
static Bool
elementsNextElement (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
	 	     CompOption      *option,
		     int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s   = findScreenAtDisplay (d, xid);

    if (s)
    {
	CompListValue   *cType, *cPath, *cIter;
	char            *string;
	int             i;
	Bool            foundHigher = FALSE;
	ElementTypeInfo *info;

	ELEMENTS_DISPLAY (d);
	ELEMENTS_SCREEN (s);

	cType = elementsGetElementType (s);
	cPath = elementsGetElementImage (s);
	cIter = elementsGetElementIter (s);

	if (!((cType->nValue  == cIter->nValue) &&
	      (cPath->nValue  == cIter->nValue)))
	{
	    compLogMessage ("elements", CompLogLevelWarn,
			    "Options are not set correctly,"
			    " cannot read this setting.");
	    return FALSE;
	}

	for (i = 0; i < cIter->nValue; i++)
	{
	    if (cIter->value[i].i > es->animIter)
	    {
		foundHigher  = TRUE;
		es->listIter = i;
		es->animIter = cIter->value[i].i;
		break;
	    }
	}

	if (!foundHigher)
	{
	    int lowest = 50;
	    es->listIter = 0;
	    for (i = 0; i < cIter->nValue; i++)
		if (cIter->value[i].i < lowest)
		    lowest = cIter->value[i].i;

	    es->animIter = lowest;
	}

	if (ed->textFunc && cType->nValue > 0)
	{
	    for (info = ed->elementTypes; info; info = info->next)
	    {
		if (!strcmp (info->name, cType->value[es->listIter].s))
		{
		    string = info->desc;
		    break;
	        }
	    }

	    if (string)
	    {
		int height;

		elementsRenderTitle (s, string);

		height = es->textData ? es->textData->height : 0;

		es->renderText    = TRUE;
		es->renderTexture =
		    createTemporaryTexture (s, cPath, cIter,
					    es->animIter, height);
		addDisplayTimeouts (s, es->ntTextures > 1);
		damageScreen (s);
	    }
	}
	else if (ed->textFunc)
	{
	    elementsRenderTitle (s, "No elements have been defined");
	    es->renderText = TRUE;
	    addDisplayTimeouts (s, es->ntTextures > 1);
	}
    }

    return FALSE;
}

static Bool
elementsPrevElement (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
	 	     CompOption      *option,
		     int             nOption)
{
    CompScreen      *s;
    Window          xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s   = findScreenAtDisplay (d, xid);

    if (s)
    {
	CompListValue   *cType, *cPath, *cIter;
	char            *string;
	int             i;
	Bool            foundLower = FALSE;
	ElementTypeInfo *info;

	ELEMENTS_DISPLAY (d);
	ELEMENTS_SCREEN (s);

	cType = elementsGetElementType (s);
	cPath = elementsGetElementImage (s);
	cIter = elementsGetElementIter (s);

	if (!((cType->nValue  == cIter->nValue) &&
	      (cPath->nValue  == cIter->nValue)))
	{
	    compLogMessage ("elements", CompLogLevelWarn,
			    "Options are not set correctly,"
			    " cannot read this setting.");
	    return FALSE;
	}

	for (i = cIter->nValue -1; i > -1; i--)
	{
	    if (cIter->value[i].i < es->animIter)
	    {
		foundLower   = TRUE;
		es->listIter = i;
		es->animIter = cIter->value[i].i;
		break;
	    }
	}

	if (!foundLower)
	{
	    int highest = 0;

	    for (i = 0; i < cIter->nValue; i++)
		if (cIter->value[i].i > highest)
		    highest = cIter->value[i].i;

	    es->animIter = highest;
	    for (i = 0; i < cIter->nValue; i++)
		if (cIter->value[i].i == highest)
		    break;

	    es->listIter = i;
	}

	if (ed->textFunc && cType->nValue > 0)
	{
	    for (info = ed->elementTypes; info; info = info->next)
	    {
		if (!strcmp (info->name, cType->value[es->listIter].s))
		{
		    string = info->desc;
		    break;
	        }
	    }

	    if (string)
	    {
		int height;

		elementsRenderTitle (s, string);

		height = es->textData ? es->textData->height : 0;

		es->renderText    = TRUE;
		es->renderTexture =
		    createTemporaryTexture (s, cPath, cIter,
					    es->animIter, height);
		addDisplayTimeouts (s, es->ntTextures > 1);
		damageScreen (s);
		
	    }
	}
	else if (ed->textFunc)
	{
	    elementsRenderTitle (s, "No elements have been defined");
	    es->renderText = TRUE;
	    addDisplayTimeouts (s, es->ntTextures > 1);
	}

    }

    return FALSE;
}

static Bool
elementsToggleSelected (CompDisplay     *d,
		        CompAction      *action,
		        CompActionState state,
	 	        CompOption      *option,
		        int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s   = findScreenAtDisplay (d, xid);

    if (s)
    {
	char             *string;
	Bool             success = TRUE;
	ElementTypeInfo  *info;
	CompListValue    *cIter  = elementsGetElementIter  (s);
	CompListValue    *cType  = elementsGetElementType  (s);
	CompListValue    *cPath  = elementsGetElementImage (s);
	CompListValue    *cCap   = elementsGetElementCap   (s);
	CompListValue    *cSize  = elementsGetElementSize  (s);
	CompListValue    *cSpeed = elementsGetElementSpeed (s);
	ElementAnimation *anim;

	ELEMENTS_DISPLAY (d);
	ELEMENTS_SCREEN (s);

	if (!((cType->nValue  == cIter->nValue) &&
	      (cPath->nValue  == cIter->nValue) &&
	      (cCap->nValue   == cIter->nValue) &&
	      (cSize->nValue  == cIter->nValue) &&
	      (cSpeed->nValue == cIter->nValue)))
	{
	    compLogMessage ("elements", CompLogLevelWarn,
			    "Options are not set correctly,"
			    " cannot read this setting.");
	    return FALSE;
	}

	if (cType->nValue < 1)
	{
	    if (ed->textFunc)
	    {
		elementsRenderTitle (s, "No elements have been defined\n");
		es->renderText = TRUE;
		addDisplayTimeouts (s, es->ntTextures > 1);
	    }
	}

	for (anim = es->animations; anim; anim = anim->next)
	{
	    if (anim->id == es->animIter)
	    {
		anim->active = !anim->active;
		break;
	    }
	}

	if (!anim)
	{
	    success = createElementAnimation (s,
					      cCap->value[es->listIter].i,
					      cType->value[es->listIter].s,
					      cSize->value[es->listIter].i,
					      cSpeed->value[es->listIter].i,
					      es->animIter);
	}
	
	if (ed->textFunc && elementsGetTitleOnToggle (s) && success)
	{
	    for (info = ed->elementTypes; info; info = info->next)
	    {
		if (!strcmp (info->name, cType->value[es->listIter].s))
		{
		    string = info->desc;
		    break;
	        }
	    }

	    if (string)
	    {
		int height;

		elementsRenderTitle (s, string);

		height = es->textData ? es->textData->height : 0;

		es->renderText    = TRUE;
		es->renderTexture =
		    createTemporaryTexture (s, cPath, cIter,
					    es->animIter, height);
		addDisplayTimeouts (s, es->ntTextures > 1);
		damageScreen (s);
		
	    }
	}
	else if (ed->textFunc && elementsGetTitleOnToggle (s) && anim)
	{
	    elementsRenderTitle (s, "Error - Element image was not found"
				    " or is invalid");
	    es->renderText = TRUE;
	    addDisplayTimeouts (s, es->ntTextures > 1);
	    damageScreen (s);
	}
	
    }

    return FALSE;
}

static GLuint
setupDisplayList (void)
{
    GLuint retval;

    retval = glGenLists (1);
    glNewList (retval, GL_COMPILE);
    glBegin (GL_QUADS);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, 0, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, 1.0, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (1.0, 1.0, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (1.0, 0, -0.0);
    glEnd ();
    glEndList ();

    return retval;
}

static void
beginRendering (CompScreen *s)
{
    ElementAnimation *anim;

    ELEMENTS_SCREEN (s);

    glEnable (GL_BLEND);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (es->needUpdate)
    {
	es->displayList = setupDisplayList ();
	es->needUpdate = FALSE;
    }

    for (anim = es->animations; anim; anim = anim->next)
    {
	int    num = anim->nElement;
	int    i;

	if (anim->nTextures > 0)
	{
	    for (i = 0; i < num; i++)
	    {
		Element *e = &anim->elements[i];
		int     nTexture = e->nTexture % anim->nTextures;

		enableTexture (s, &anim->texture[nTexture].tex,
			       COMP_TEXTURE_FILTER_GOOD);

		glColor4f (1.0, 1.0, 1.0, e->opacity);
		glTranslatef (e->x, e->y, e->z);
		glRotatef (e->rAngle, 0, 0, 1);
		glCallList (anim->texture[nTexture].dList);
		glRotatef (-e->rAngle, 0, 0, 1);
		glTranslatef (-e->x, -e->y, -e->z);

		disableTexture (s, &anim->texture[nTexture].tex);
	    }
	}
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}


static Bool
elementsPaintOutput (CompScreen              *s,
		     const ScreenPaintAttrib *sa,
		     const CompTransform     *transform,
		     Region                  region,
		     CompOutput              *output,
		     unsigned int            mask)
{
    Bool status;

    ELEMENTS_SCREEN (s);

    if (es->animations && elementsGetOverWindows (s))
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (es, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (es, s, paintOutput, elementsPaintOutput);

    if (es->renderText || (es->animations && elementsGetOverWindows (s)))
    {
	CompTransform sTransform = *transform;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	if (es->animations && elementsGetOverWindows (s))
	    beginRendering (s);
	if (es->renderText)
	    elementsDrawTitle (s);

	glPopMatrix ();
    }

    return status;
}


static Bool
elementsDrawWindow (CompWindow           *w,
		    const CompTransform  *transform,
		    const FragmentAttrib *attrib,
		    Region               region,
		    unsigned int         mask)
{
    Bool       status;
    CompScreen *s = w->screen;

    ELEMENTS_SCREEN (s);

    UNWRAP (es, s, drawWindow);
    status = (*s->drawWindow) (w, transform, attrib, region, mask);
    WRAP (es, s, drawWindow, elementsDrawWindow);

    if (es->animations && (w->type & CompWindowTypeDesktopMask) &&
	!elementsGetOverWindows (s))
    {
	beginRendering (s);
    }
    return status;
}

void
initiateElement (CompScreen       *s,
		 ElementAnimation *anim,
		 Element          *e)
{
    e->x  = 0;
    e->y  = 0;
    e->z  = elementsMmRand (-elementsGetScreenDepth (s), 0.1, 5000);
    e->dz = elementsMmRand (-500, 500, 500000);
    e->rAngle = elementsMmRand (-1000, 1000, 50);
    e->rSpeed = elementsMmRand (-2100, 2100, 700);

    e->opacity  = 1.0f;
    e->nTexture = 0;
    e->ptr      = NULL;

    if (anim->properties->initiate)
	(*anim->properties->initiate) (s, e);
}

void
elementMove (CompScreen       *s,
	     Element          *e,
	     ElementAnimation *anim,
	     int              updateDelay)
{
    if (anim->properties->move)
	(*anim->properties->move) (s, anim, e, updateDelay);

}

static Bool
stepPositions (void *closure)
{
    CompScreen       *s = (CompScreen *) closure;
    ElementAnimation *anim;

    ELEMENTS_SCREEN (s);

    if (!es->animations)
	return TRUE;

    for (anim = es->animations; anim; anim = anim->next)
    {
	int i;

	if (anim->active)
	{
	    for (i = 0; i < anim->nElement; i++)
		elementTestCreate (s, &anim->elements[i], anim);
	}
	else
	{
	    if (elementTestOffscreen (s, anim))
	    {
		for (i = 0; i < anim->nTextures; i++)
		{
		    finiTexture (s, &anim->texture[i].tex);
		    glDeleteLists (anim->texture[i].dList, 1);
		}

		free (anim->elements);
		free (anim->texture);
		elementsDeleteAnimation (s, anim);
		break;
	    }
	}
    }

    if (!elementsGetOverWindows (s))
    {
	CompWindow *w;

	for (w = s->windows; w; w = w->next)
	{
	    if (w->type & CompWindowTypeDesktopMask)
		addWindowDamage (w);
	}
    }
    else
    {
	damageScreen (s);
    }

    return TRUE;
}

void
updateElementTextures (CompScreen *s,
		       Bool       changeTextures)
{
    ElementAnimation *anim;

    ELEMENTS_SCREEN (s);

    for (anim = es->animations; anim; anim = anim->next)
    {
	int           i, iter, nElement, size,speed;
	char          *type;
	CompListValue *cType  = elementsGetElementType  (s);
	CompListValue *cPath  = elementsGetElementImage (s);
	CompListValue *cCap   = elementsGetElementCap   (s);
	CompListValue *cSize  = elementsGetElementSize  (s);
	CompListValue *cSpeed = elementsGetElementSpeed (s);
	CompListValue *cIter  = elementsGetElementIter  (s);
	Element *e;
	Bool    initiate = FALSE;

	if (!((cType->nValue  == cIter->nValue) &&
	      (cPath->nValue  == cIter->nValue) &&
	      (cCap->nValue   == cIter->nValue) &&
	      (cSize->nValue  == cIter->nValue) &&
	      (cSpeed->nValue == cIter->nValue)))
	{
	    compLogMessage ("elements", CompLogLevelWarn,
			    "Options are not set correctly,"
			    " cannot read this setting.");
	    return;
	}

	iter     = anim->id;
	nElement = cCap->value[anim->id - 1].i;
	type     = cType->value[anim->id - 1].s;
	size     = cSize->value[anim->id - 1].i;
	speed    = cSpeed->value[anim->id - 1].i;

	for (i = 0; i < anim->nTextures; i++)
	{
	    finiTexture (s, &anim->texture[i].tex);
	    glDeleteLists (anim->texture[i].dList, 1);
	}

	if (strcmp (type, anim->type))
	{
	    int i;

	    /* discard old memory and create new one for anim's type */
	    free (anim->type);
	    anim->type = strdup (type);

	    if (!elementsPropertiesForAnimation (s->display, anim, type))
	    {
		compLogMessage ("elements", CompLogLevelWarn,
				"Could not find element movement pattern %s",
				type);
	    }

	    if (anim->properties->fini)
	    {
		e = anim->elements;

		for (i = 0; i < anim->nElement; i++, e++)
		    (*anim->properties->fini) (s, e);
	    }
	    initiate = TRUE;
	}

	if (textureToAnimation (s, anim, cPath, cIter, size, iter))
	{
	    if (nElement != anim->nElement)
	    {
		Element *newElements;

		newElements = realloc (anim->elements,
						sizeof (Element) * nElement);
		if (newElements)
		{
		    anim->elements = newElements;
		    anim->nElement = nElement;
		}
		else
		    nElement = anim->nElement;

		initiate = TRUE;
	    }

	    anim->size  = size;
	    anim->speed = speed;
	    e = anim->elements;

	    if (initiate)
		for (; nElement--; e++)
		    initiateElement (s, anim, e);
	}
    }
}

static ElementsBaseFunctions elementsFunctions =
{
    .newElementType    = elementsCreateNewElementType,
    .removeElementType = elementsRemoveElementType,
    .getRand	       = elementsGetRand,
    .mmRand	       = elementsMmRand,
    .boxing	       = elementsGetEBoxing,
    .depth	       = elementsGetEDepth
};

static void
elementsScreenOptionChanged (CompScreen            *s,
			     CompOption            *opt,
			     ElementsScreenOptions num)
{
    switch (num)
    {
	case ElementsScreenOptionElementType:
	case ElementsScreenOptionElementImage:
	case ElementsScreenOptionElementCap:
	case ElementsScreenOptionElementSize:
	case ElementsScreenOptionElementSpeed:
	case ElementsScreenOptionElementRotate:
	{

	    ELEMENTS_SCREEN (s);

	    es->needUpdate = TRUE;
	    updateElementTextures (s, FALSE);

	}
	break;
    	case ElementsScreenOptionUpdateDelay:
	{
	    ELEMENTS_SCREEN (s);

	    if (es->timeoutHandle)
		compRemoveTimeout (es->timeoutHandle);
	    es->timeoutHandle =
		compAddTimeout (elementsGetUpdateDelay (s),
				(float) elementsGetUpdateDelay (s) * 1.2,
				stepPositions, s);
	}
	break;
	default:
	    break;
    }
}

static Bool
elementsInitScreen (CompPlugin *p,
		    CompScreen *s)
{
    ElementsScreen *es;
    int            delay;
    int 	   lowest = 50;
    int		   i;
    CompListValue  *cIter = elementsGetElementIter (s);

    ELEMENTS_DISPLAY (s->display);

    es = calloc (1, sizeof (ElementsScreen));
    if (!es)
	return FALSE;

    es->needUpdate    = FALSE;
    es->listIter      = 0;
    es->animations    = NULL;
    es->textData      = NULL;
    es->renderText    = FALSE;
    es->renderTexture = FALSE;
    es->renderTimeout = 0;
    es->switchTimeout = 0;
    es->eTexture      = NULL;
    for (i = 0; i < cIter->nValue; i++)
	if (cIter->value[i].i < lowest)
	    lowest = cIter->value[i].i;

    es->animIter = lowest;

    elementsSetElementTypeNotify (s, elementsScreenOptionChanged);
    elementsSetElementImageNotify (s, elementsScreenOptionChanged);
    elementsSetElementSizeNotify (s, elementsScreenOptionChanged);
    elementsSetElementSpeedNotify (s, elementsScreenOptionChanged);
    elementsSetElementCapNotify (s, elementsScreenOptionChanged);
    elementsSetElementRotateNotify (s, elementsScreenOptionChanged);
    elementsSetUpdateDelayNotify (s, elementsScreenOptionChanged);

    es->displayList = setupDisplayList ();

    delay             = elementsGetUpdateDelay (s);
    es->timeoutHandle = compAddTimeout (delay, (float) delay * 1.2,
					stepPositions, s);

    WRAP (es, s, paintOutput, elementsPaintOutput);
    WRAP (es, s, drawWindow, elementsDrawWindow);

    s->base.privates[ed->screenPrivateIndex].ptr = es;

    updateElementTextures (s, TRUE);

    return TRUE;
}

static void
elementsFiniScreen (CompPlugin *p,
		    CompScreen *s)
{
    ELEMENTS_SCREEN (s);

    if (es->timeoutHandle)
	compRemoveTimeout (es->timeoutHandle);

    if (es->renderTimeout)
    	compRemoveTimeout (es->renderTimeout);

    elementsFreeTitle (s);
    
    if (es->eTexture)
    {
	int i;
	for (i = 0; i < es->ntTextures; i++)
	{
    	    finiTexture (s, &es->eTexture[i].tex);
	    glDeleteLists (es->eTexture[i].dList, 1);
	}
    	free (es->eTexture);
    }

    elementsRemoveElementType (s, "autumn");
    elementsRemoveElementType (s, "fireflies");
    elementsRemoveElementType (s, "snow");
    elementsRemoveElementType (s, "stars");
    elementsRemoveElementType (s, "bubbles");

    UNWRAP (es, s, paintOutput);
    UNWRAP (es, s, drawWindow);

    free (es);
}

static Bool
elementsInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    ElementsDisplay *ed;
    CompOption      *abi, *index;
    int             idx;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    ed = malloc (sizeof (ElementsDisplay));
    if (!ed)
	return FALSE;

    ed->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ed->screenPrivateIndex < 0)
    {
	free (ed);
	return FALSE;
    }

    if (checkPluginABI ("text", TEXT_ABIVERSION) &&
	getPluginDisplayIndex (d, "text", &idx))
    {
	ed->textFunc = d->base.privates[idx].ptr;
    }
    else
    {
	compLogMessage ("elements", CompLogLevelWarn,
			"No compatible text plugin found.");
	ed->textFunc = NULL;
    }

    ed->elementTypes = NULL;

    abi   = elementsGetAbiOption (d);
    index = elementsGetIndexOption (d);

    abi->value.i   = ELEMENTS_ABIVERSION;
    index->value.i = functionsPrivateIndex;

    elementsSetNextElementKeyInitiate (d, elementsNextElement);
    elementsSetPrevElementKeyInitiate (d, elementsPrevElement);
    elementsSetToggleSelectedKeyInitiate (d, elementsToggleSelected);

    d->base.privates[displayPrivateIndex].ptr   = ed;
    d->base.privates[functionsPrivateIndex].ptr = &elementsFunctions;

    elementsCreateNewElementType (d, "autumn", "Autumn",
				  initiateAutumnElement,
				  autumnMove,
				  autumnFini);

    elementsCreateNewElementType (d, "fireflies", "Fireflies",
				  initiateFireflyElement,
				  fireflyMove,
				  fireflyFini);

    elementsCreateNewElementType (d, "snow", "Snow",
				  initiateSnowElement,
				  snowMove,
				  snowFini);

    elementsCreateNewElementType (d, "stars",  "Stars",
				  initiateStarElement,
				  starMove,
				  starFini);

    elementsCreateNewElementType (d, "bubbles", "Bubbles",
				  initiateBubbleElement,
				  bubbleMove,
				  bubbleFini);

    return TRUE;
}


static void
elementsFiniDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    ELEMENTS_DISPLAY (d);

    freeScreenPrivateIndex (d, ed->screenPrivateIndex);
    free (ed);
}

static Bool
elementsInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    functionsPrivateIndex = allocateDisplayPrivateIndex ();
    if (functionsPrivateIndex < 0)
    {
	freeDisplayPrivateIndex (displayPrivateIndex);
	return FALSE;
    }

     return TRUE;
}

static void
elementsFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
    freeDisplayPrivateIndex (functionsPrivateIndex);
}

static CompBool
elementsInitObject(CompPlugin *p,
		   CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) elementsInitDisplay,
	(InitPluginObjectProc) elementsInitScreen
     };

    RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void
elementsFiniObject(CompPlugin *p,
		   CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) elementsFiniDisplay,
	(FiniPluginObjectProc) elementsFiniScreen
    };

    DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}

CompPluginVTable elementsVTable = {
    "elements",
    0,
    elementsInit,
    elementsFini,
    elementsInitObject,
    elementsFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &elementsVTable;
}
