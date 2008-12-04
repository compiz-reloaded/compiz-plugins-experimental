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

/*
 * Note - The textures in animations are only
 *        displayed from the first frame.
 */

#define _GNU_SOURCE /* for strndup */

#include <errno.h>
#include <pthread.h>

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "cubemodel-internal.h"
#include "cubemodel_options.h"


/**************************
* Gets path from object   *
* and file name from file *
* and returns full path   *
* to the file             *
**************************/

static char *
findPath (char *object,
	  char *file)
{
    char *filePath; /* string containing /full/path/to/file */
    int  i;

    if (!file || !object)
	return NULL;

    if (file[0] == '/')
	return strdup (filePath);

    filePath = strdup (object);
    if (!filePath)
	return NULL;

    for (i = strlen (filePath) - 1; i >= 0; i--)
    {
	if (filePath[i] == '/')
	{
	    filePath[i + 1]='\0'; /* end string at last /
				     (gives path to object) */
	    break;
	}
    }

    filePath = realloc (filePath,
  			sizeof (char) *
			(strlen (filePath) + strlen (file) + 1));
    if (!filePath)
	return NULL;

    strcat (filePath, file);

    return filePath;
}

static int
addNumToString (char         **sp,
                unsigned int size,
                int          offset,
                char         *post,
                unsigned int x,
                unsigned int maxNumZeros)
{
    int  c = 0;
    int  numZeros = 0;
    char *s = *sp;
    int  i = x, j;

    while (i != 0)
    {
	c++;
	i /= 10;
    }

    if (maxNumZeros > c)
	numZeros = maxNumZeros - c;

    j = offset + c + numZeros + strlen (post) + 4 + 1;
    if (j > size)
    {
	size = j;
	s = *sp = realloc (*sp, size * sizeof (char));
    }

    snprintf (s + offset, size - offset, "%0*d%s.obj", maxNumZeros, x, post);

    return size;
}

static int
addVertex (unsigned int **indices,
	   int		nUniqueIndices,
	   int		iTexture,
	   int		iNormal)
{
    int i, len;

    if (*indices == NULL)
    {
	*indices = malloc (4 * sizeof (int));
	(*indices)[0] = 1; /* store size of array as 1st element */
	(*indices)[1] = iTexture;
	(*indices)[2] = iNormal;
	(*indices)[3] = nUniqueIndices; /* ptr to new unique vertex index */

	return -1; /* new vertex/texture/normal */
    }

    len = (*indices)[0];
    for (i = 0; i < len; i++)
    {
	if ((*indices)[1 + 3 * i] == iTexture &&
	    (*indices)[2 + 3 * i] == iNormal)
	{
	    return (*indices)[3 + 3 * i]; /* found same vertex/texture/normal
					     before */
	}
    }

    *indices = realloc (*indices, (1 + 3 * ((*indices)[0] + 1)) *
                         sizeof (int));
    (*indices)[1 + 3 * (*indices)[0]] = iTexture;
    (*indices)[2 + 3 * (*indices)[0]] = iNormal;
    (*indices)[3 + 3 * (*indices)[0]] = nUniqueIndices;
    (*indices)[0]++;

    return -1;
}

static Bool
compileDList (CompScreen      *s,
	      CubemodelObject *data)
{
    if (!data->animation && data->finishedLoading && !data->compiledDList)
    {
	data->dList = glGenLists (1);
	glNewList (data->dList, GL_COMPILE);

	glDisable (GL_CULL_FACE);
	glEnable  (GL_NORMALIZE);
	glEnable  (GL_DEPTH_TEST);

	glDisable (GL_COLOR_MATERIAL);

	cubemodelDrawVBOModel (s, data,
	                       (float *) data->reorderedVertex[0],
	                       (float *) data->reorderedNormal[0]);
	glEndList ();

	data->compiledDList = TRUE;

	return TRUE;
    }
    return FALSE;
}

static void
loadMaterials (CompScreen      *s,
               CubemodelObject *data,
               char            *approxPath,
               char            *filename,
               mtlStruct       **material,
               int             *nMat)
{
    int i;

    /* getLine stuff */
    char *strline    = NULL;
    char *tmpType;

    int tempBufferSize = 2048; /* get character data from files
				  in these size chunks */
    fileParser *fParser = NULL;


    mtlStruct *currentMaterial = NULL;

    char *mtlFilename;
    FILE *mtlfp;

    char delim[] = { ' ', '\t', '\0' };

    int nMaterial = *nMat;

    if (nMaterial == 0)
	*material = NULL;

    mtlFilename = findPath (approxPath, filename);
    if (!mtlFilename)
	return;

    mtlfp = fopen (mtlFilename, "r");

    if (mtlFilename)
	free (mtlFilename);

    if (!mtlfp)
    {
	compLogMessage ("cubemodel", CompLogLevelWarn,
	                "Failed to open material file : %s", mtlFilename);
	return;
    }

    fParser = initFileParser (mtlfp, tempBufferSize);

    /* now read all the materials in the mtllib referenced file */

    while ((strline = getLine (fParser)))
    {
	char *tmpPtr[3] = { NULL, NULL, NULL }; /* used to check numerical
						   parameters*/
	float tmpNum[3] = {0, 0, 0 };
	float tmpIllum = 100;

	tmpType = strsep2 (&strline, delim);
	if (!strline || !tmpType)
	    continue;

	if (!strcmp (tmpType, "newmtl"))
	{
	    tmpType = strsep2 (&strline, delim);
	    if (!tmpType)
		continue;

	    *material = realloc (*material, sizeof (mtlStruct) *
	                         (nMaterial + 1));
	    currentMaterial = &((*material)[nMaterial]);

	    nMaterial++;

	    currentMaterial->name = strdup (tmpType);

	    /* set defaults */
	    currentMaterial->Ns[0] = 100;
	    currentMaterial->Ni[0] = 1;
	    currentMaterial->illum = 2;

	    for (i = 0; i < 3; i++)
	    {
		currentMaterial->Ka[i] = 0.2;
		currentMaterial->Kd[i] = 0.8;
		currentMaterial->Ks[i] = 1;
	    }
	    currentMaterial->Ka[3] = 1;
	    currentMaterial->Kd[3] = 1;
	    currentMaterial->Ks[3] = 1;

	    currentMaterial->map_Ka = -1;
	    currentMaterial->map_Kd = -1;
	    currentMaterial->map_Ks = -1;
	    currentMaterial->map_d  = -1;

	    currentMaterial->map_params = -1;
	}

	if (!currentMaterial)
	    continue;

	for (i = 0; i < 3; i++)
	{
	    tmpPtr[i] = strsep2 (&strline, delim);
	    if (!tmpPtr[i])
		break;

	    tmpNum[i] = atof (tmpPtr[i]);

	    if (i == 0)
		tmpIllum = atoi (tmpPtr[i]);
	}

	if (!strcmp (tmpType, "Ns"))
	{
	    currentMaterial->Ns[0] = tmpNum[0];
	}
	else if (!strcmp (tmpType, "Ka"))
	{
	    for (i = 0; i < 3; i++)
		currentMaterial->Ka[i] = tmpNum[i];
	}
	else if (!strcmp (tmpType, "Kd"))
	{
	    for (i = 0; i < 3; i++)
		currentMaterial->Kd[i] = tmpNum[i];
	}
	else if (!strcmp (tmpType, "Ks"))
	{
	    for (i = 0; i < 3; i++)
		currentMaterial->Ks[i] = tmpNum[i];
	}
	else if (!strcmp (tmpType, "Ni"))
	{
	    currentMaterial->Ni[0] = tmpNum[0];
	}
	else if (!strcmp (tmpType, "d") || !strcmp(tmpType,"Tr"))
	{
	    currentMaterial->Ka[3] = tmpNum[0];
	    currentMaterial->Kd[3] = tmpNum[0];
	    currentMaterial->Ks[3] = tmpNum[0];
	}
	else if (!strcmp (tmpType, "illum"))
	{
	    currentMaterial->illum = tmpIllum;
	}
	else if (!strcmp (tmpType, "map_Ka") || !strcmp (tmpType, "map_Kd") ||
		 !strcmp (tmpType, "map_Ks") || !strcmp (tmpType, "map_d" ) )
	{
	    char *tmpName = NULL;

	    if (!data->tex)
	    {
		data->tex = malloc (sizeof (CompTexture));
		if (!data->tex)
		{
		    compLogMessage ("cubemodel", CompLogLevelWarn,
		                    "Error allocating texture memory");
		    break;
		}
		data->texName = NULL;

		data->texWidth  = malloc (sizeof (unsigned int));
		if (!data->texWidth)
		{
		    free (data->tex);
		    data->tex = NULL;
		    break;
		}
		data->texHeight = malloc (sizeof (unsigned int));
		if (!data->texHeight)
		{
		    free (data->tex);
		    free (data->texWidth);
		    data->tex = NULL;
		    break;
		}

		data->nTex = 0;
	    }
	    else
	    {
		Bool match = FALSE;

		if (!data->texName && data->nTex > 0)
		    continue;

		for (i = 0; i < data->nTex; i++)
		{
		    if (!data->texName[i])
			break;

		    if (!strcmp (tmpPtr[0], data->texName[i]))
		    {
			if (!strcmp (tmpType, "map_Ka"))
			    currentMaterial->map_Ka = i;
			else if (!strcmp (tmpType, "map_Kd"))
			    currentMaterial->map_Kd = i;
			else if (!strcmp (tmpType, "map_Ks"))
			    currentMaterial->map_Ks = i;
			else if (!strcmp (tmpType, "map_d"))
			    currentMaterial->map_d = i;

			currentMaterial->width  = data->texWidth[i];
			currentMaterial->height = data->texHeight[i];

			currentMaterial->map_params = i;

			match = TRUE;
			break;
		    }
		}

		if (match)
		    continue;

		data->tex      = realloc (data->tex,
		                          sizeof (CompTexture) *
		                          (data->nTex + 1));
		data->texWidth = realloc (data->texWidth,
		                          sizeof (unsigned int) *
		                          (data->nTex + 1));
		data->texHeight= realloc (data->texHeight,
		                          sizeof (unsigned int) *
		                          (data->nTex + 1));
	    }

	    initTexture (s, &(data->tex[data->nTex]));

	    if (!data->tex)
	    {
		compLogMessage ("cubemodel", CompLogLevelWarn,
		                "CompTexture is not malloced properly");
	    }
	    else
	    {
		tmpName = findPath (approxPath, tmpPtr[0]);
		if (!readImageToTexture (s,
		                         &(data->tex[data->nTex]), /*texture*/
		                         tmpName, /*file*/
		                         &(currentMaterial->width),  /*width*/
		                         &(currentMaterial->height)))/*height*/
		{
		    compLogMessage ("cubemodel", CompLogLevelWarn,
		                   "Failed to load image: %s", tmpName);

		    finiTexture (s, &(data->tex[data->nTex]));
		}
		else
		{
		    data->texName  = realloc (data->texName,
					      sizeof (char *) *
					      (data->nTex + 1));

		    data->texName[data->nTex] = strdup (tmpPtr[0]);

		    if (!strcmp (tmpType, "map_Ka"))
			currentMaterial->map_Ka = data->nTex;
		    else if (!strcmp (tmpType, "map_Kd"))
			currentMaterial->map_Kd = data->nTex;
		    else if (!strcmp (tmpType, "map_Ks"))
			currentMaterial->map_Ks = data->nTex;
		    else if (!strcmp (tmpType, "map_d"))
			currentMaterial->map_d = data->nTex;

		    data->texWidth[data->nTex]  = currentMaterial->width;
		    data->texHeight[data->nTex] = currentMaterial->height;

		    currentMaterial->map_params = data->nTex;

		    data->nTex++;
		}
	    }

	    if (tmpName)
		free (tmpName);
	    tmpName = NULL;
	}
    }

    freeFileParser(fParser);

    fclose (mtlfp);

    *nMat = nMaterial;
}

static Bool
initLoadModelObject (CompScreen      *s,
		     CubemodelObject *modelData)
{
    char *filename = modelData->filename;
    char *post     = modelData->post;

    int size            = modelData->size;
    int lenBaseFilename = modelData->lenBaseFilename;
    int startFileNum    = modelData->startFileNum;
    int maxNumZeros     = modelData->maxNumZeros;

    /* getLine stuff */
    char *strline    = NULL;
    char *tmpType;

    int tempBufferSize = 4 * 1024; /* get character data from files
				      in these size chunks */
    fileParser *fParser = NULL;

    int nVertex = 0;
    int nNormal = 0;
    int nTexture = 0;
    int nIndices = 0;

    const char delim[] = { ' ', '\t', '\0' };

    FILE *fp;

    modelData->nMaterial[0] = 0;
    modelData->material[0] = NULL;

    /* First pass - count how much data we need to store and
     *		  - load the materials from any mtllib references.
     */

    if (modelData->animation)
	size = addNumToString (&filename, size, lenBaseFilename, post,
	                       startFileNum, maxNumZeros);

    fp = fopen (filename, "r");
    if (!fp)
    {
	compLogMessage ("cubemodel", CompLogLevelWarn,
	                "Failed to open model file - %s", filename);
	return FALSE;
    }

    fParser = initFileParser (fp, tempBufferSize);

    while ((strline = getLine (fParser)))
    {
	tmpType = strsep2 (&strline, delim);
	if (!strline || !tmpType)
	    continue;

	if (!strcmp (tmpType, "v"))
	    nVertex++;
	else if (!strcmp (tmpType, "vn"))
	    nNormal++;
	else if (!strcmp (tmpType, "vt"))
	    nTexture++;
	else if (!strcmp (tmpType, "f") || !strcmp (tmpType, "fo") ||
		 !strcmp (tmpType, "p") || !strcmp (tmpType, "l") )
	{
	    while ((tmpType = strsep2 (&strline, delim)))
		nIndices++;
	}
	else if (!strcmp (tmpType, "mtllib"))
	{
	    while ((tmpType = strsep2 (&strline, delim)))
	    {
		if (tmpType[0] == '\0') /*skip "" as it can't be a valid file*/
		    break;

		loadMaterials (s, modelData, filename, tmpType,
		               &(modelData->material[0]),
		               &(modelData->nMaterial[0]));
	    }
	}
    }

    modelData->reorderedVertex[0]  = malloc (sizeof (vect3d) * nIndices);
    modelData->reorderedTexture[0] = malloc (sizeof (vect2d) * nIndices);
    modelData->reorderedNormal[0]  = malloc (sizeof (vect3d) * nIndices);

    modelData->indices = malloc (sizeof (unsigned int *) * nIndices);
    modelData->reorderedVertexBuffer  = malloc (sizeof (vect3d) * nIndices);
    modelData->reorderedTextureBuffer = malloc (sizeof (vect2d) * nIndices);
    modelData->reorderedNormalBuffer  = malloc (sizeof (vect3d) * nIndices);

    modelData->nVertex  = nVertex;
    modelData->nNormal  = nNormal;
    modelData->nTexture = nTexture;
    modelData->nIndices = nIndices;

    freeFileParser (fParser);

    return TRUE;
}

static Bool
loadModelObject (CubemodelObject *modelData)
{
    int i, j;

    char *filename = modelData->filename;
    char *post     = modelData->post;

    int size            = modelData->size;
    int lenBaseFilename = modelData->lenBaseFilename;
    int startFileNum    = modelData->startFileNum;
    int maxNumZeros     = modelData->maxNumZeros;

    /* getLine stuff */
    char *strline    = NULL;
    char *tmpType;

    int tempBufferSize = 4 * 1024; /* get character data from files
				      in these size chunks */
    fileParser *fParser = NULL;

    int fileCounter = modelData->fileCounter;

    int nVertex=0;
    int nNormal=0;
    int nTexture=0;
    int nIndices=0;
    int nUniqueIndices = 0;

    unsigned int **tmpIndices = NULL; /* reorder indices per vertex
					 (store alternating corresponding
					 textures/normals) */
    vect3d *vertex  = NULL;
    vect3d *normal  = NULL;
    vect2d *texture = NULL;

    const char delim[]= { ' ', '\t', '\0' };

    FILE *fp;

    int nGroups  = 0;

    int oldPolyCount = 0;
    Bool oldUsingNormal = FALSE;
    Bool oldUsingTexture = FALSE;

    /* store size of each array */
    int sVertex = 0;
    int sTexture = 0;
    int sNormal = 0;
    int sIndices = 0;

    int fc;

    fParser = initFileParser (NULL, tempBufferSize);

    for (fc = 0; fc < fileCounter; fc++)
    {
	int lastLoadedMaterial = -1;
	int prevLoadedMaterial = -1;

	if (modelData->animation)
	    size = addNumToString (&filename, size, lenBaseFilename,
	                           post, startFileNum+fc, maxNumZeros);

	fp = fopen(filename, "r");
	if (!fp)
	{
	    compLogMessage ("cubemodel", CompLogLevelWarn,
	                    "Failed to open model file - %s", filename);
	    free (normal);
	    free (tmpIndices);
	    free (texture);
	    free (vertex);
	    freeFileParser (fParser);
	    return FALSE;
	}

	updateFileParser (fParser, fp);

	if (fc == 0)
	{
	    nVertex  = modelData->nVertex;
	    nTexture = modelData->nTexture;
	    nNormal  = modelData->nNormal;
	    nIndices = modelData->nIndices;

	    sVertex  = nVertex;
	    sTexture = nTexture;
	    sNormal  = nNormal;
	    sIndices = nIndices;
	}
	else
	{
	    sIndices = modelData->nIndices;
	}

	modelData->reorderedVertex[fc]  = malloc (sizeof (vect3d) * sIndices);
	modelData->reorderedTexture[fc] = malloc (sizeof (vect2d) * sIndices);
	modelData->reorderedNormal[fc]  = malloc (sizeof (vect3d) * sIndices);

	if (fc == 0)
	{
	    vertex  = malloc (sizeof (vect3d) * nVertex);
	    texture = malloc (sizeof (vect2d) * nTexture);
	    normal  = malloc (sizeof (vect3d) * nNormal);

	    modelData->indices = malloc (sizeof (unsigned int *) * nIndices);
	    modelData->reorderedVertexBuffer  = malloc (sizeof (vect3d) *
	                                                nIndices);
	    modelData->reorderedTextureBuffer = malloc (sizeof (vect2d) *
	                                                nIndices);
	    modelData->reorderedNormalBuffer  = malloc (sizeof (vect3d) *
	                                                nIndices);

	    modelData->nVertex  = nVertex;
	    modelData->nNormal  = nNormal;
	    modelData->nTexture = nTexture;
	    modelData->nIndices = nIndices;

	    tmpIndices = malloc (sizeof (unsigned int **) * sVertex);
	    for (i = 0; i < sVertex; i++)
		tmpIndices[i] = NULL;
	}
	else
	{
	    for (i = 0; i < sVertex; i++)
	    {
		if (tmpIndices[i])
		    tmpIndices[i][0] = 0; /* set length to 0 */
	    }
	}

	nVertex  = 0;
	nNormal  = 0;
	nTexture = 0;
	nIndices = 0;
	nUniqueIndices = 0;

	/* Second pass - fill arrays
	 * 	       - reorder data and store into vertex/normal/texture
	 * 		 buffers
	 */

	while ((strline = getLine (fParser)))
	{
	    int complexity = 0;
	    int polyCount  = 0;
	    Bool updateGroup  = FALSE;
	    Bool usingNormal  = FALSE;
	    Bool usingTexture = FALSE;

	    tmpType = strsep2 (&strline, delim);
	    if (!strline || !tmpType)
		continue;

	    if (!strcmp (tmpType, "v"))
	    {
		if (sVertex <= nVertex)
		{
		    sVertex++;
		    vertex     = realloc (vertex, sizeof (vect3d) * sVertex);
		    tmpIndices = realloc (tmpIndices,
					  sizeof (vect3d) * sVertex);
		    tmpIndices[sVertex - 1] = NULL;
		}

		for (i = 0; i < 3; i++)
		{
		    tmpType = strsep2 (&strline, delim);
		    if (!tmpType)
		    {
			vertex[nVertex].r[0] = 0;
			vertex[nVertex].r[1] = 0;
			vertex[nVertex].r[2] = 0;
			break;
		    }
		    vertex[nVertex].r[i] = atof (tmpType);
		}
		nVertex++;
	    }
	    else if (!strcmp (tmpType, "vn"))
	    {
		if (sNormal <= nNormal)
		{
		    sNormal++;
		    normal = realloc (normal, sizeof (vect3d) * sNormal);
		}

		for (i = 0; i < 3; i++)
		{
		    tmpType = strsep2 (&strline, delim);
		    if (!tmpType)
		    {
			normal[nNormal].r[0] = 0;
			normal[nNormal].r[1] = 0;
			normal[nNormal].r[2] = 1;
			break;
		    }
		    normal[nNormal].r[i] = atof(tmpType);
		}
		nNormal++;
	    }
	    else if (!strcmp (tmpType, "vt"))
	    {
		if (sTexture <= nTexture)
		{
		    sVertex++;
		    texture = realloc (texture, sizeof (vect3d) * sTexture);
		}

		/* load the 1D/2D coordinates for textures */
		for (i = 0; i < 2; i++)
		{
		    tmpType = strsep2 (&strline, delim);
		    if (!tmpType)
		    {
			if (i == 0)
			    texture[nTexture].r[0] = 0;

			texture[nTexture].r[1] = 0;
			break;
		    }
		    texture[nTexture].r[i] = atof (tmpType);
		}
		nTexture++;
	    }
	    else if (!strcmp (tmpType, "usemtl") && fc == 0)
	    {
		/* parse mtl file(s) and load specified material */
		tmpType = strsep2 (&strline, delim);
		if (!tmpType)
		    continue;

		for (j = 0; j < modelData->nMaterial[fc]; j++)
		{
		    if (!strcmp (tmpType, modelData->material[fc][j].name))
		    {
			lastLoadedMaterial = j;
			updateGroup = TRUE;
			break;
		    }
		}
	    }
	    else if (!strcmp (tmpType, "f") || !strcmp (tmpType, "fo") ||
		     !strcmp (tmpType, "p") || !strcmp (tmpType, "l"))
	    {
		char *tmpPtr; /* used to check value of vertex/texture/normal */

		if (!strcmp (tmpType, "l"))
		    complexity = 1;
		else if (!strcmp (tmpType, "f") || !strcmp (tmpType, "fo"))
		    complexity = 2;

		while ((tmpType = strsep2 (&strline, delim)))
		{
		    int vertexIndex  = -1;
		    int textureIndex = -1;
		    int normalIndex  = -1;
		    int tmpInd;

		    tmpPtr = strsep (&tmpType, "/");
		    if (tmpPtr)
		    {
			vertexIndex = atoi (tmpPtr);
			if (vertexIndex > 0)
			{
			    /* skip vertex index past last in obj file */
			    if (vertexIndex > modelData->nVertex)
				break;
			    vertexIndex--;
			}
			else if (vertexIndex < 0)
			{
			    vertexIndex += nVertex;

			    /* skip vertex index < 0 in obj file */
			    if (vertexIndex < 0)
				break;
			}
			else /* skip vertex index of 0 in obj file */
			    break;
		    }
		    else
			break;

		    tmpPtr = strsep (&tmpType, "/");
		    if (tmpPtr && complexity != 0)
		    {
			/* texture */

			if (strcmp (tmpPtr, ""))
			{
			    textureIndex = atoi (tmpPtr);

			    if (textureIndex > 0)
			    {
				 /* skip normal index past last in obj file */
				if (textureIndex > modelData->nTexture)
				    break;
				textureIndex--;
			    }
			    else if (textureIndex < 0)
			    {
				textureIndex += nTexture;

				/* skip texture index < 0 in obj file */
				if (textureIndex < 0)
				    break;
			    }
			    else /* skip texture index of 0 in obj file */
				break;

			    usingTexture = TRUE;
			}

			tmpPtr = strsep (&tmpType, "/");
			if (tmpPtr && strcmp (tmpPtr, "") && complexity == 2)
			{
			    /* normal */

			    normalIndex = atoi (tmpPtr);

			    if (normalIndex > 0)
			    {
				/* skip normal index past last in obj file */
				if (normalIndex > modelData->nNormal)
				    break;
				normalIndex--;
			    }
			    else if (normalIndex < 0)
			    {
				normalIndex += nNormal;

				/* skip normal index < 0 in obj file */
				if (normalIndex < 0)
				    break;
			    }
			    else /* skip normal index of 0 in obj file */
				break;

			    usingNormal = TRUE;
			}
		    }

		    /* reorder vertices/textures/normals */

		    tmpInd = addVertex (&tmpIndices[vertexIndex],
					nUniqueIndices, textureIndex,
					normalIndex);
		    if (tmpInd < 0)
		    {
			if (nUniqueIndices >= sIndices)
			{
			    sIndices++;
			    modelData->reorderedVertex[fc]  =
				realloc (modelData->reorderedVertex[fc],
				         sizeof (vect3d) * sIndices);
			    modelData->reorderedTexture[fc] =
				realloc (modelData->reorderedTexture[fc],
				         sizeof (vect2d) * sIndices);
			    modelData->reorderedNormal[fc]  =
				realloc (modelData->reorderedNormal[fc],
				         sizeof (vect3d) * sIndices);
			}

			if (vertexIndex < nVertex)
			{
			    memcpy (modelData->reorderedVertex[fc]
				    [nUniqueIndices].r,
				    vertex[vertexIndex].r,
				    3 * sizeof (float));
			}
			else
			{
			    for (i = 0; i < 3; i++)
				modelData->reorderedVertex[fc]
				    [nUniqueIndices].r[i] = 0;
			}

			if (textureIndex >= 0 && textureIndex < nTexture)
			{
			    memcpy (modelData->reorderedTexture[fc]
				    [nUniqueIndices].r,
				    texture[textureIndex].r,
				    2 * sizeof (float));


			    /* scale as per 1st loaded texture for
			     * that material (1st frame)*/

			    if (lastLoadedMaterial >=0 && modelData->tex)
			    {
				mtlStruct *currentMaterial =
				    &(modelData->material[fc]
				      [lastLoadedMaterial]);

				if (currentMaterial->map_params >= 0)
				{
				    CompMatrix *ct =
					&((&(modelData->tex
					[currentMaterial->map_params]))->
					matrix);

				    modelData->reorderedTexture[fc]
				    [nUniqueIndices].r[0] =
					COMP_TEX_COORD_X (ct,
					(currentMaterial->width - 1) *
					(texture[textureIndex].r[0]));
				    modelData->reorderedTexture[fc]
				    [nUniqueIndices].r[1] =
					COMP_TEX_COORD_Y (ct,
					(currentMaterial->height - 1) *
					(1 - texture[textureIndex].r[1]));
				}
			    }
			}
			else
			{
			    modelData->reorderedTexture[fc]
			    [nUniqueIndices].r[0] = 0;
			    modelData->reorderedTexture[fc]
			    [nUniqueIndices].r[1] = 0;
			}

			if (normalIndex >= 0 && normalIndex < nNormal)
			    memcpy (modelData->reorderedNormal[fc]
				    [nUniqueIndices].r,
				    normal[normalIndex].r,
				    3 * sizeof (float));
			else
			{
			    modelData->reorderedNormal[fc]
				[nUniqueIndices].r[0] = 0;
			    modelData->reorderedNormal[fc]
				[nUniqueIndices].r[1] = 0;
			    modelData->reorderedNormal[fc]
				[nUniqueIndices].r[2] = 1;
			}

			modelData->indices[nIndices] = nUniqueIndices;
			nUniqueIndices++;
		    }
		    else
			modelData->indices[nIndices] = tmpInd;

		    nIndices++;
		    polyCount++;
		}

		updateGroup = TRUE;
	    }

	    if (updateGroup && fc == 0)
	    {
		if (polyCount !=0 &&
		    (polyCount != oldPolyCount       ||
		     usingNormal != oldUsingNormal   ||
		     usingTexture != oldUsingTexture ||
		     lastLoadedMaterial != prevLoadedMaterial))
		{
		    oldPolyCount = polyCount;
		    oldUsingTexture = usingTexture;
		    oldUsingNormal  = usingNormal;
		    prevLoadedMaterial = lastLoadedMaterial;

		    modelData->group = realloc (modelData->group,
						(nGroups + 1) *
						sizeof (groupIndices));

		    modelData->group[nGroups].polyCount = polyCount;
		    modelData->group[nGroups].complexity = complexity;
		    modelData->group[nGroups].startV = nIndices - polyCount;

		    modelData->group[nGroups].materialIndex =
			lastLoadedMaterial;

		    if (nGroups > 0)
			modelData->group[nGroups - 1].numV = nIndices -
			    polyCount - modelData->group[nGroups - 1].startV;

		    modelData->group[nGroups].texture = usingTexture;
		    modelData->group[nGroups].normal  = usingNormal;

		    nGroups++;
		}
	    }
	}

	if (nGroups != 0 && fc == 0)
	    modelData->group[nGroups - 1].numV = nIndices -
		modelData->group[nGroups - 1].startV;

	if (fc == 0)
	    modelData->nUniqueIndices = nUniqueIndices;

	fclose (fp);

	if (fc == 0 && modelData->animation)
	{ /* set up 1st frame for display */
	    vect3d *reorderedVertex  = modelData->reorderedVertex[0];
	    vect3d *reorderedNormal  = modelData->reorderedNormal[0];

	    for (i = 0; i < modelData->nUniqueIndices; i++)
	    {
		for (j = 0; j < 3; j++)
		{
		    modelData->reorderedVertexBuffer[i].r[j] =
			reorderedVertex[i].r[j];
		    modelData->reorderedNormalBuffer[i].r[j] =
			reorderedNormal[i].r[j];
		}
	    }
	}

    }

    modelData->nGroups = nGroups;

    if (vertex)
	free (vertex);
    if (normal)
	free (normal);
    if (texture)
	free (texture);


    if (tmpIndices)
    {
	for (i = 0; i < sVertex; i++)
	    if (tmpIndices[i])
		free (tmpIndices[i]);
	free (tmpIndices);
    }

    freeFileParser (fParser);

    modelData->finishedLoading = TRUE;

    return TRUE;
}

static void *
loadModelObjectThread (void *ptr)
{
    CubemodelObject *modelData = (CubemodelObject *) ptr;
    modelData->threadRunning = TRUE;

    loadModelObject (modelData);

    modelData->updateAttributes = TRUE;
    modelData->threadRunning = FALSE;

    pthread_exit (NULL);
}

Bool
cubemodelAddModelObject (CompScreen      *s,
			 CubemodelObject *modelData,
			 char            *file,
			 float           *translate,
                         float           *rotate,
			 float           rotateSpeed,
                         float           *scale,
			 float           *color,
                         Bool            animation,
			 float           fps)
{
    int  i, size;
    int  fileCounter = 0; /* value checked in cubemodelDeleteModelObject */
    int  lenFilename;
    int  startFileNum = 0;
    int  maxNumZeros = 6;
    int  lenBaseFilename;
    FILE *fp;
    Bool flag;

    modelData->fileCounter = 0;
    modelData->updateAttributes = FALSE;

    if (!file)
	return FALSE;
    if (!strlen (file))
	return FALSE;

    modelData->rotate[0] = rotate[0]; /* rotation angle */
    modelData->rotate[1] = rotate[1]; /* rotateX */
    modelData->rotate[2] = rotate[2]; /* rotateY */
    modelData->rotate[3] = rotate[3]; /* rotateZ */

    modelData->translate[0] = translate[0]; /* translateX */
    modelData->translate[1] = translate[1]; /* translateY */
    modelData->translate[2] = translate[2]; /* translateZ */

    modelData->scaleGlobal = scale[0];
    modelData->scale[0] = scale[1]; /* scaleX */
    modelData->scale[1] = scale[2]; /* scaleY */
    modelData->scale[2] = scale[3]; /* scaleZ */

    modelData->rotateSpeed = rotateSpeed;
    modelData->animation   = animation;
    modelData->fps         = fps;
    modelData->time        = 0;

    if (!color)
    {
	modelData->color[0] = 1;
	modelData->color[1] = 1;
	modelData->color[2] = 1;
	modelData->color[3] = 1;
    }
    else
    {
	modelData->color[0] = color[0]; /* R */
	modelData->color[1] = color[1]; /* G */
	modelData->color[2] = color[2]; /* B */
	modelData->color[3] = color[3]; /* alpha */
    }

    /* set to NULL so delete will not crash for threads */
    modelData->reorderedVertex        = NULL;
    modelData->reorderedTexture	      = NULL;
    modelData->reorderedNormal        = NULL;
    modelData->nMaterial 	      = NULL;
    modelData->material 	      = NULL;
    modelData->tex      	      = NULL;
    modelData->texName   	      = NULL;
    modelData->texWidth   	      = NULL;
    modelData->texHeight   	      = NULL;
    modelData->reorderedVertexBuffer  = NULL;
    modelData->reorderedTextureBuffer = NULL;
    modelData->reorderedNormalBuffer  = NULL;
    modelData->indices 		      = NULL;
    modelData->group                  = NULL;


    modelData->compiledDList = FALSE;
    modelData->finishedLoading = FALSE;

    modelData->threadRunning = FALSE;

    modelData->post = NULL;
    modelData->filename = NULL;

    lenFilename = strlen (file);
    size = lenFilename + 1 + 4;

    if (lenFilename > 3)
    {
	if (strstr (file + lenFilename - 4, ".obj"))
	{
	    lenFilename -= 4;
	    size -= 4;
	}
    }

    modelData->filename = calloc (size, sizeof (char));
    if (!modelData->filename)
        return FALSE;

    strncpy (modelData->filename, file, lenFilename);
    if (!modelData->animation)
	strcat (modelData->filename, ".obj");

    lenBaseFilename = lenFilename;

    if (modelData->animation)
    {
	char *start, *numbers = NULL;
	char *post = modelData->filename + lenFilename;

	start = strrchr (modelData->filename, '/');
	if (!start)
	    start = modelData->filename;

	start++;
	while (*start)
	{
	    if (*start >= '0' && *start <= '9')
	    {
		if (!numbers)
		    numbers = start;
	    }
	    else if (numbers)
	    {
		post = start;
		break;
	    }
	    start++;
	}

	if (numbers)
	{
	    lenBaseFilename = numbers - modelData->filename;
	    maxNumZeros     = post - numbers;

	    modelData->post = strdup (post);
	    if (!modelData->post)
		return FALSE;

	    strncpy (modelData->filename, file, lenBaseFilename);
	    startFileNum = strtol (numbers, NULL, 10);
	}
	else
	{
	    modelData->animation = FALSE;
	    strcat (modelData->filename, ".obj");
	}
    }

    /* verify existence of files and/or check for animation frames */

    do
    {
	if (modelData->animation)
	    size = addNumToString (&modelData->filename, size,
				   lenBaseFilename, modelData->post,
				   startFileNum + fileCounter, maxNumZeros);

	fp = fopen (modelData->filename, "r");
	if (fp)
	{
	    printf ("opened %s\n", modelData->filename);

	    fclose (fp);
	    fileCounter++;
	}
    }
    while (modelData->animation && fp);

    modelData->fileCounter = fileCounter;
    if (!fileCounter)
    {
	compLogMessage ("cubemodel", CompLogLevelWarn,
			"Failed to open model file : %s", modelData->filename);

	if (modelData->filename)
	    free (modelData->filename);
	if (modelData->post)
	    free (modelData->post);

	return FALSE;
    }

    modelData->reorderedVertex  = malloc (sizeof (vect3d *) * fileCounter);
    modelData->reorderedTexture = malloc (sizeof (vect2d *) * fileCounter);
    modelData->reorderedNormal  = malloc (sizeof (vect3d *) * fileCounter);

    modelData->reorderedVertexBuffer  = NULL;
    modelData->reorderedTextureBuffer = NULL;
    modelData->reorderedNormalBuffer  = NULL;

    modelData->material  = malloc (sizeof (mtlStruct *) * fileCounter);
    modelData->nMaterial = malloc (sizeof (int) * fileCounter);

    for (i = 0; i < fileCounter; i++)
    {
	modelData->material[i]  = 0;
	modelData->nMaterial[i] = 0;
    }

    modelData->tex = NULL;
    modelData->texName = NULL;
    modelData->nTex = 0;
    modelData->texWidth = NULL;
    modelData->texHeight = NULL;

    modelData->indices = NULL;
    modelData->group = NULL;

    modelData->size = size;
    modelData->lenBaseFilename = lenBaseFilename;
    modelData->startFileNum = startFileNum;
    modelData->maxNumZeros = maxNumZeros;

    flag = initLoadModelObject (s, modelData);

    if (flag)
    {
	if  (cubemodelGetConcurrentLoad (s))
	{
	    int iret;

	    modelData->threadRunning = TRUE;

	    iret = pthread_create (&modelData->thread, NULL,
				   loadModelObjectThread,
				   (void*) modelData);
	    if (!iret)
		return TRUE;

	    compLogMessage ("cubemodel", CompLogLevelWarn,
			    "Error creating thread: %s\n"
			    "Trying single threaded approach", file);
	    modelData->threadRunning = FALSE;
	}

	flag = loadModelObject (modelData);
    }

    return flag;
}

Bool
cubemodelDeleteModelObject (CompScreen      *s,
			    CubemodelObject *data)
{
    int i, fc;

    if (!data)
	return FALSE;

    if (data->fileCounter == 0)
	return FALSE;

    if (data->threadRunning)
    {
	int ret;

	ret = pthread_join (data->thread, NULL); /* not best in all
						    circumstances */
	if (ret)
	{
	    compLogMessage ("cubemodel", CompLogLevelWarn,
			    "Could not synchronize with thread.\n"
			    "Possible memory leak)");
	    return FALSE;
	}
    }

    if (data->filename)
	free (data->filename);

    if (data->post)
	free (data->post);

    if (!data->animation && data->compiledDList)
	glDeleteLists (data->dList, 1);

    for (fc = 0; fc < data->fileCounter; fc++)
    {
	if (data->reorderedVertex && data->reorderedVertex[fc])
	    free (data->reorderedVertex[fc]);
	if (data->reorderedTexture && data->reorderedTexture[fc])
	    free (data->reorderedTexture[fc]);
	if (data->reorderedNormal && data->reorderedNormal[fc])
	    free (data->reorderedNormal[fc]);

	if (data->nMaterial)
	{
	    for (i = 0; i< data->nMaterial[fc]; i++)
	    {
		if (data->material[fc][i].name)
		    free (data->material[fc][i].name);
	    }
	}

	if (data->material && data->material[fc])
	    free(data->material[fc]);
    }

    if (data->tex)
    {
	for (i = 0; i < data->nTex; i++)
	{
	    if (&(data->tex[i]) != NULL)
		finiTexture (s, &(data->tex[i]));
	}
	free (data->tex);
    }

    if (data->texName)
    {
	for (i = 0; i < data->nTex; i++)
	{
	    if (data->texName[i])
		free (data->texName[i]);
	}
    }

    if (data->texWidth)
	free (data->texWidth);
    if (data->texHeight)
	free (data->texHeight);

    if (data->reorderedVertex)
	free (data->reorderedVertex);
    if (data->reorderedTexture)
	free (data->reorderedTexture);
    if (data->reorderedNormal)
	free (data->reorderedNormal);
    if (data->material)
	free (data->material);

    if (data->reorderedVertexBuffer)
	free (data->reorderedVertexBuffer);
    if (data->reorderedTextureBuffer)
	free (data->reorderedTextureBuffer);
    if (data->reorderedNormalBuffer)
	free (data->reorderedNormalBuffer);

    if (data->indices)
	free (data->indices);
    if (data->group)
	free (data->group);

    return TRUE;
}

Bool
cubemodelDrawModelObject (CompScreen      *s,
			  CubemodelObject *data,
			  float           scale)
{
    if (!data->fileCounter || !data->finishedLoading)
	return FALSE;

    if (!data->animation && !data->compiledDList)
	compileDList (s, data);

    /* Rotate, translate and scale  */

    glTranslatef (data->translate[0], data->translate[2], data->translate[1]);

    glScalef (data->scaleGlobal * data->scale[0],
	      data->scaleGlobal * data->scale[1],
	      data->scaleGlobal * data->scale[2]);

    glScalef (scale, scale, scale);

    glRotatef (data->rotate[0], data->rotate[1],
	       data->rotate[2], data->rotate[3]);

    glDisable (GL_CULL_FACE);
    glEnable (GL_NORMALIZE);
    glEnable (GL_DEPTH_TEST);

    glEnable (GL_COLOR_MATERIAL);
    glColor4fv (data->color);

    if (data->animation)
    {
	cubemodelDrawVBOModel (s, data,
	                       (float *) data->reorderedVertexBuffer,
	                       (float *) data->reorderedNormalBuffer);
    }
    else
    {
	glCallList (data->dList);
    }

    return TRUE;
}

Bool
cubemodelUpdateModelObject (CompScreen      *s,
			    CubemodelObject *data,
			    float           time)
{
    int i, j;

    if (!data->fileCounter || !data->finishedLoading)
	return FALSE;

    if (!data->animation && !data->compiledDList)
	compileDList (s, data);

    data->rotate[0] += 360 * time * data->rotateSpeed;
    data->rotate[0] = fmodf (data->rotate[0], 360.0f);

    if (data->animation && data->fps)
    {
	float  t, dt, dt2;
	int    ti, ti2;
	vect3d *reorderedVertex, *reorderedVertex2;
	vect3d *reorderedNormal, *reorderedNormal2;

	data->time += time * data->fps;
	data->time = fmodf (data->time, (float) data->fileCounter);

	t = data->time;
	if (t < 0)
	    t += (float) data->fileCounter;

	ti  = (int) t;
	ti2 = (ti + 1) % data->fileCounter;
	dt  = t - ti;
	dt2 = 1 - dt;

	reorderedVertex  = data->reorderedVertex[ti];
	reorderedVertex2 = data->reorderedVertex[ti2];
	reorderedNormal  = data->reorderedNormal[ti];
	reorderedNormal2 = data->reorderedNormal[ti2];

	for (i = 0; i < data->nUniqueIndices; i++)
	{
	    for (j = 0; j < 3; j++)
	    {
		data->reorderedVertexBuffer[i].r[j] =
		    dt2 * (reorderedVertex[i].r[j]) +
		    dt  * (reorderedVertex2[i].r[j]);
		data->reorderedNormalBuffer[i].r[j] =
		    dt2 * (reorderedNormal[i].r[j]) +
		    dt  * (reorderedNormal2[i].r[j]);
	    }
	}
    }

    return TRUE;
}

static void
setMaterial (const float *shininess,
	     const float *ambient,
	     const float *diffuse,
	     const float *specular)
{
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, specular);
}

Bool
cubemodelDrawVBOModel (CompScreen      *s,
		       CubemodelObject *data,
		       float           *vertex,
		       float           *normal)
{
    groupIndices *group;
    int          i, j;

    static const float white[4] = { 1.0, 1.0, 1.0, 1.0 };
    static const float black[4] = { 0.0, 0.0, 0.0, 0.0 };
    static const float defaultShininess[1] = { 100 };

    float *v = vertex;
    float *n = normal;
    float *t = (float *) data->reorderedTexture[0];

    CompTexture *currentTexture = NULL;
    int         currentTextureIndex = -1; /* last loaded texture -
					     to prevent multiple loadings */

    Bool prevNormal = TRUE, prevTexture = FALSE;

    int prevMaterialIndex = -1;

    int ambientTextureIndex     = -1;
    int diffuseTextureIndex     = -1;
    int specularTextureIndex    = -1;
    int transparentTextureIndex = -1;

    const float *ambient   = white;
    const float *diffuse   = white;
    const float *specular  = white;
    const float *shininess = defaultShininess;

    glVertexPointer (3, GL_FLOAT, 0, v);
    glNormalPointer (GL_FLOAT, 0, n);
    glTexCoordPointer (2, GL_FLOAT, 0, t);

    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_NORMAL_ARRAY);
    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glDisable (GL_TEXTURE_2D);

    for (i = 0; i < data->nGroups; i++)
    {
	GLenum cap = GL_QUADS;

	group = &(data->group[i]);
	if (group->polyCount < 1)
	    continue;

	if (group->polyCount == 3)
	    cap = GL_TRIANGLES;
	if (group->polyCount == 2 || group->complexity == 1)
	    cap = GL_LINE_LOOP;
	if (group->polyCount == 1 || group->complexity == 0)
	    cap = GL_POINTS;

	if (group->normal && !prevNormal)
	{
	    glEnableClientState (GL_NORMAL_ARRAY);
	    prevNormal = TRUE;
	}
	else if (!group->normal && prevNormal)
	{
	    glDisableClientState (GL_NORMAL_ARRAY);
	    prevNormal = FALSE;
	}

	if (group->materialIndex >= 0)
	{
	    if (group->materialIndex != prevMaterialIndex)
	    {
		glDisable (GL_COLOR_MATERIAL);

		ambientTextureIndex     =
		    data->material[0][group->materialIndex].map_Ka;
		diffuseTextureIndex     =
		    data->material[0][group->materialIndex].map_Kd;
		specularTextureIndex    =
		    data->material[0][group->materialIndex].map_Ks;
		transparentTextureIndex =
		    data->material[0][group->materialIndex].map_d;

		ambient   = data->material[0][group->materialIndex].Ka;
		diffuse   = data->material[0][group->materialIndex].Kd;
		specular  = data->material[0][group->materialIndex].Ks;
		shininess = data->material[0][group->materialIndex].Ns;

		setMaterial (shininess, ambient, diffuse, specular);

		switch (data->material[0][group->materialIndex].illum) {
		case 0:
		    glDisable (GL_LIGHTING);
		    break;
		case 1:
		    specular = black;
		default:
		    glEnable (GL_LIGHTING);
		}
	    }
	    prevMaterialIndex = group->materialIndex;
	}

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (group->texture && transparentTextureIndex >= 0)
	{
	    if (!prevTexture)
	    {
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glEnable (GL_TEXTURE_2D);
		prevTexture = TRUE;
	    }

	    if (transparentTextureIndex >= 0)
	    {
		if (!currentTexture ||
		    transparentTextureIndex != currentTextureIndex)
		{
		    currentTextureIndex = transparentTextureIndex;
		    if (currentTexture)
			disableTexture (s, currentTexture);

		    currentTexture =  &(data->tex[transparentTextureIndex]);
		    if (currentTexture)
		    {
			glEnable (currentTexture->target);
			enableTexture (s, currentTexture,
				       COMP_TEXTURE_FILTER_GOOD);
		    }
		}

		glBlendFunc (GL_SRC_ALPHA, GL_ONE);
		setMaterial (shininess, white, white, white);

		if (data->group[i].polyCount < 5)
		    glDrawElements (cap, group->numV, GL_UNSIGNED_INT,
				    data->indices + group->startV);
		else
		{
		    for (j = 0; j < group->numV / group->polyCount; j++)
		    {
			glDrawElements (GL_POLYGON,
					group->polyCount,
					GL_UNSIGNED_INT,
					data->indices + group->startV +
					j * group->polyCount);
		    }
		}

		glBlendFunc (GL_ONE_MINUS_DST_ALPHA, GL_SRC_COLOR);
		setMaterial (shininess, ambient, diffuse, specular);
	    }
	}

	if (group->texture && diffuseTextureIndex >= 0)
	{
	    if (!prevTexture)
	    {
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glEnable (GL_TEXTURE_2D);
		prevTexture = TRUE;
	    }

	    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, white);

	    if (!currentTexture || diffuseTextureIndex != currentTextureIndex)
	    {
		currentTextureIndex = diffuseTextureIndex;
		if (currentTexture)
		    disableTexture (s, currentTexture);

		currentTexture =  &(data->tex[diffuseTextureIndex]);
		if (currentTexture)
		{
		    glEnable (currentTexture->target);
		    enableTexture (s, currentTexture, COMP_TEXTURE_FILTER_GOOD);
		}
	    }
	}
	else
	{
	    if (prevTexture)
	    {
		glDisable (GL_TEXTURE_2D);
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		prevTexture = FALSE;
	    }

	    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	}

	if (data->group[i].polyCount < 5)
	    glDrawElements (cap, group->numV, GL_UNSIGNED_INT,
			    data->indices + group->startV);
	else
	{
	    for (j = 0; j < group->numV/group->polyCount; j++)
	    {
		glDrawElements (GL_POLYGON, group->polyCount, GL_UNSIGNED_INT,
				data->indices + group->startV +
				j * group->polyCount);
	    }
	}
    }

    if (currentTexture)
	disableTexture (s, currentTexture);

    glDisable (GL_TEXTURE_2D);
    glDisableClientState (GL_NORMAL_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    return TRUE;
}


