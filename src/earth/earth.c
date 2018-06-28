/*
 * Compiz Earth plugin
 *
 * Copyright : (C) 2010 by Maxime Wack
 * E-mail    : maximewack(at)free(dot)fr
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

#include "earth.h"
#include "earth_options.h"

int earthDisplayPrivateIndex;
int cubeDisplayPrivateIndex;

static void
makeSphere (GLdouble radius, GLboolean inside)
{
	GLint i,j;
	GLfloat PI = 3.14159265358979323846;
	GLfloat sinCache1a[65];
	GLfloat cosCache1a[65];
	GLfloat sinCache2a[65];
	GLfloat cosCache2a[65];
	GLfloat sinCache1b[65];
	GLfloat cosCache1b[65];
	GLfloat sinCache2b[65];
	GLfloat cosCache2b[65];
	GLfloat angle;
	GLfloat zLow, zHigh;
	GLfloat sintemp1 = 0.0, sintemp2 = 0.0, sintemp3 = 0.0, sintemp4 = 0.0;
	GLfloat costemp3 = 0.0, costemp4 = 0.0;

	for (i = 0; i < 64; i++)
	{
		angle = 2* PI * i / 64;
		sinCache1a[i] = sinf (angle);
		cosCache1a[i] = cosf (angle);
		sinCache2a[i] = sinCache1a[i];
		cosCache2a[i] = cosCache1a[i];
	}

	for (j = 0; j <= 64; j++)
	{
		angle = PI * j / 64;
		if (!inside)
		{
			sinCache2b[j] = sinf (angle);
			cosCache2b[j] = cosf (angle);
		}
		else
		{
			sinCache2b[j] = -sinf (angle);
			cosCache2b[j] = -cosf (angle);
		}
		sinCache1b[j] = radius * sinf (angle);
		cosCache1b[j] = radius * cosf (angle);
	}
	sinCache1b[0] = 0;
	sinCache1b[64] = 0;

	sinCache1a[64] = sinCache1a[0];
	cosCache1a[64] = cosCache1a[0];
	sinCache2a[64] = sinCache2a[0];
	cosCache2a[64] = cosCache2a[0];

	for (j = 0; j < 64; j++)
	{
		zLow = cosCache1b[j];
		zHigh = cosCache1b[j+1];

		sintemp1 = sinCache1b[j];
		sintemp2 = sinCache1b[j+1];

		if (!inside)
		{
			sintemp3 = sinCache2b[j+1];
			costemp3 = cosCache2b[j+1];
			sintemp4 = sinCache2b[j];
			costemp4 = cosCache2b[j];
		}
		else
		{
			sintemp3 = sinCache2b[j];
			costemp3 = cosCache2b[j];
			sintemp4 = sinCache2b[j+1];
			costemp4 = cosCache2b[j+1];
		}


		glBegin (GL_QUAD_STRIP);
		for (i = 0; i <= 64; i++)
		{
			glNormal3f(sinCache2a[i] * sintemp3, cosCache2a[i] * sintemp3, costemp3);
			if (!inside)
			{
				glTexCoord2f(1 - (float) i / 64, 1 - (float) (j+1) / 64);
				glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh);
			}
			else
			{
				glTexCoord2f(1 - (float) i / 64, 1 - (float) j / 64);
				glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow);
			}

			glNormal3f(sinCache2a[i] * sintemp4, cosCache2a[i] * sintemp4, costemp4);
			if (!inside)
			{
				glTexCoord2f(1 - (float) i / 64, 1 - (float) j / 64);
				glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow);
			}
			else
			{
				glTexCoord2f(1 - (float) i / 64, 1 - (float) (j+1) / 64);
				glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh);
			}
		}
		glEnd ();
	}
}

static char*
LoadSource (char* filename)
{
	char* src = NULL;   /* shader source code */
	FILE* fp = NULL;	/* file */
	long size;		  /* file size */
	long i;			 /* count */

	fp = fopen (filename, "r");
	if (fp == NULL)
	{
		compLogMessage ("earth", CompLogLevelWarn,
						"Unable to load %s for reading", filename);
		return NULL;
	}

	fseek (fp, 0, SEEK_END);
	size = ftell (fp);

	rewind (fp);

	src = malloc (size+1);
	if (src == NULL)
	{
		fclose (fp);
		compLogMessage ("earth", CompLogLevelError,
						"Memory allocation error!");
		return NULL;
	}

	for (i = 0; i < size; i++)
		src[i] = fgetc (fp);

	src[size] = '\0';

	fclose (fp);

	return src;
}

static void*
LoadTexture_t (void* threaddata)
{
	TexThreadData* data = (TexThreadData*) threaddata;
	CompScreen* s = data->s;
	int num = data->num;
	struct stat st;
	char* texfile;

	EARTH_SCREEN (s);

	switch (num)
	{
	case DAY:
		assert (asprintf (&texfile, "%s", "earth/day.png") ==
									strlen ("earth/day.png"));
	break;
	case NIGHT:
		assert (asprintf (&texfile, "%s", "earth/night.png") ==
									strlen ("earth/night.png"));
	break;
	case SKY:
		assert (asprintf (&texfile, "%s", "earth/skydome.png") ==
									strlen ("earth/skydome.png"));
	break;
	case CLOUDS:
		assert (asprintf (&texfile, "%s%s", getenv ("HOME"),
									"/.compiz/images/clouds.png") ==
									strlen (getenv ("HOME")) +
									strlen ("/.compiz/images/clouds.png"));
		if (stat (texfile, &st) == 0 && S_ISREG (st.st_mode))
			break;
		else
		{
			free(texfile);
			assert (asprintf (&texfile, "%s", "earth/clouds.png") ==
									strlen ("earth/clouds.png"));
		}
	break;
	}

	es->tex[num] = createTexture (s);
	if (readImageFromFile (s->display, texfile, &es->imagedata[num].width, &es->imagedata[num].height, &es->imagedata[num].image))
		compLogMessage ("earth", CompLogLevelInfo,
						"Loaded texture %s", texfile);
	else
	{
		compLogMessage ("earth", CompLogLevelWarn,
						"Failed to load %s", texfile);
		es->imagedata[num].image = NULL;
	}

	free (texfile);

	return NULL;
}

static void*
DownloadClouds_t (void* threaddata)
{
	CloudsThreadData* data = (CloudsThreadData*) threaddata;
	EARTH_SCREEN (data->s);

	data->started = 1;
	data->finished = 0;

	/* cloudsfile initialization */
	es->cloudsfile.stream = NULL;

	compLogMessage ("earth", CompLogLevelInfo,
					"Downloading %s to %s", data->url, es->cloudsfile);

	/* Download the jpg file */
	if (es->curlhandle)
		curl_easy_perform (es->curlhandle);

	if (es->cloudsfile.stream)
		fclose(es->cloudsfile.stream);

	data->finished = 1;
	return NULL;
}

static void
CreateShaders (EarthScreen* es)
{
	struct stat st;
	GLint status = 0;
	/* Shader support */
	glewInit ();
	if (glewIsSupported ("GL_VERSION_2_0"))
		es->shadersupport = GL_TRUE;
	else
	{
		compLogMessage ("earth", CompLogLevelWarn,
						"GL version 2.0 required for shader support");
		goto out;
	}

	if (stat (DATADIR "/earth/earth.vert", &st) == 0 && S_ISREG (st.st_mode))
		es->vertfile[EARTH] = DATADIR "/earth/earth.vert";
	else
	{
		compLogMessage ("earth", CompLogLevelWarn, "Failed to stat %s",
										DATADIR "/earth/earth.vert");
		goto out;
	}

	if (stat (DATADIR "/earth/earth.frag", &st) == 0 && S_ISREG (st.st_mode))
		es->fragfile[EARTH] = DATADIR "/earth/earth.frag";
	else
	{
		compLogMessage ("earth", CompLogLevelWarn, "Failed to stat %s",
										DATADIR "/earth/earth.frag");
		goto out;
	}

	/* Shader creation, loading and compiling */
	es->vert[EARTH] = glCreateShader (GL_VERTEX_SHADER);
	es->frag[EARTH] = glCreateShader (GL_FRAGMENT_SHADER);

	es->vertsource[EARTH] = LoadSource (es->vertfile[EARTH]);
	glShaderSource (es->vert[EARTH], 1, (const GLchar**)&es->vertsource[EARTH], NULL);

	es->fragsource[EARTH] = LoadSource (es->fragfile[EARTH]);
	glShaderSource (es->frag[EARTH], 1, (const GLchar**)&es->fragsource[EARTH], NULL);

	glCompileShader (es->vert[EARTH]);
	glCompileShader (es->frag[EARTH]);

	/* Check for compile errors */
	glGetObjectParameterivARB(es->vert[EARTH], GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (!status)
	{
		compLogMessage ("earth", CompLogLevelWarn, "Failed to compile vertex shader");
		goto out;
	}
	glGetObjectParameterivARB(es->frag[EARTH], GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (!status)
	{
		compLogMessage ("earth", CompLogLevelWarn, "Failed to compile fragment shader");
		goto out;
	}

	/* Program creation, attaching and linking */
	es->prog[EARTH] = glCreateProgram ();

	glAttachShader (es->prog[EARTH], es->vert[EARTH]);
	glAttachShader (es->prog[EARTH], es->frag[EARTH]);

	glLinkProgram (es->prog[EARTH]);

	/* Check for link errors */
	glGetObjectParameterivARB(es->prog[EARTH], GL_OBJECT_LINK_STATUS_ARB, &status);
	if (!status)
	{
		compLogMessage ("earth", CompLogLevelWarn, "Failed to link shader program");
	}

out:
	/* Cleanup */
	free (es->vertsource[EARTH]);
	free (es->fragsource[EARTH]);
}

static void
DeleteShaders (EarthScreen* es)
{
	if (!es->shadersupport)
		return;

	glDetachShader(es->prog[EARTH], es->vert[EARTH]);
	glDetachShader(es->prog[EARTH], es->frag[EARTH]);

	glDeleteShader(es->vert[EARTH]);
	glDeleteShader(es->frag[EARTH]);

	glDeleteProgram(es->prog[EARTH]);
}

static void
CreateLists (EarthScreen* es)
{
	int i;
	es->list[0] = glGenLists (4);

	for (i = 0; i < 4; i++)
	{
		GLdouble radius;
		GLboolean inside;

		switch (i)
		{
			case SUN:
				radius = 0.1;
				inside = TRUE;
			break;
			case EARTH:
				radius = 0.75;
				inside = FALSE;
			break;
			case CLOUDS:
				radius = 0.76;
				inside = FALSE;
			break;
			case SKY:
				radius = 10;
				inside = TRUE;
			break;
		}

		es->list[i] = es->list[0] + i;

		glNewList (es->list[i], GL_COMPILE);
		makeSphere (radius, inside);
		glEndList ();
	}
}

static size_t
write_clouds_file(void *buffer, size_t size, size_t nmemb, void *stream)
{
	CloudsFile* out = (CloudsFile*) stream;
	if (out && !out->stream)
	{
		/* open file for writing */
		out->stream = fopen (out->filename, "wb");

		if(!out->stream)
			return -1; /* failure, can't open file to write */
	}

	return fwrite(buffer, size, nmemb, out->stream);
}

static char*
TransformClouds (CompScreen* s)
{
	struct stat st;
	char* imagefile;
	char* pngfile;
	void* jpgdata;
	void* pngdata;
	char* p_jpgdata;
	char* p_pngdata;
	int width, height;
	int h, w;

	/* Load the jpgfile from disk */
	assert (asprintf (&imagefile, "%s/%s", getenv ("HOME"),
								".compiz/images/clouds.jpg") ==
								strlen (getenv ("HOME")) +
								strlen ("/.compiz/images/clouds.jpg"));
	if (stat (imagefile, &st) != 0 || !S_ISREG (st.st_mode))
	{
		free(imagefile);
		assert (asprintf (&imagefile, "%s", "clouds.jpg") == strlen ("clouds.jpg"));
	}

	if (!readImageFromFile (s->display, imagefile, &width, &height, &jpgdata))
	{
		compLogMessage ("earth", CompLogLevelWarn,
							"Failed to load %s", imagefile);
		free (imagefile);
		return NULL;
	}

	p_jpgdata = (char*) jpgdata;
	/* Adjust alpha channel */
	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++)
		{
			int pos = h * width + w;
			#if __BYTE_ORDER == __BIG_ENDIAN
			p_jpgdata[(pos * 4) + 0] = p_jpgdata[(pos * 4) + 1];
			#else
			p_jpgdata[(pos * 4) + 3] = p_jpgdata[(pos * 4) + 1];
			#endif
		}
	}

	/* Flip image vertically */
	p_pngdata = malloc (width * height * 4);

	for (h = 0; h < height; h++)
		memcpy (&p_pngdata[h * width * 4], &p_jpgdata[(height - h) * width * 4], width * 4);

	free (jpgdata);
	pngdata = (void*) p_pngdata;

	/* Write in the pngfile */
	assert (asprintf (&pngfile, "%s%s", getenv ("HOME"), "/.compiz/images/clouds.png") ==
						strlen (getenv ("HOME")) + strlen ("/.compiz/images/clouds.png"));
	if (!writeImageToFile (s->display, "", pngfile, "png", width, height, pngdata))
		compLogMessage ("earth", CompLogLevelWarn,
							"Failed to write %s", pngfile);
	else
		compLogMessage ("earth", CompLogLevelInfo,
							"Transformed %s to %s", imagefile, pngfile);

	/* Clean */
	free (pngdata);
	free (imagefile);
	return pngfile;
}

static void
earthScreenOptionChanged (CompScreen		*s,
			  CompOption		*opt,
			  EarthScreenOptions	num)
{
	EARTH_SCREEN (s);
	switch (num)
	{
	case EarthScreenOptionLatitude:
		es->lat = earthGetLatitude (s);
	break;
	case EarthScreenOptionLongitude:
		es->lon = earthGetLongitude (s);
	break;
	case EarthScreenOptionTimezone:
		es->tz = earthGetTimezone (s);
	break;
	case EarthScreenOptionClouds:
		es->clouds = earthGetClouds (s);
	break;
	case EarthScreenOptionRotation:
		es->rotspeed = earthGetRotation (s);
	break;
	case EarthScreenOptionCloudsUrl:
		es->clouds_url_changed = TRUE;
		es->cloudsthreaddata.url = earthGetCloudsUrl (s);
		curl_easy_setopt (es->curlhandle, CURLOPT_URL, earthGetCloudsUrl (s));
	break;
	case EarthScreenOptionEarthSize:
		es->earth_size = earthGetEarthSize (s);
	break;
	case EarthScreenOptionShaders:
		es->shaders = earthGetShaders (s);
		if (opt->value.b == TRUE)
		{
			es->light[EARTH].specular[0] = 0.5;
			es->light[EARTH].specular[1] = 0.5;
			es->light[EARTH].specular[2] = 0.4;
			es->light[EARTH].specular[3] = 1;
		}
		else
		{
			es->light[EARTH].specular[0] = 0;
			es->light[EARTH].specular[1] = 0;
			es->light[EARTH].specular[2] = 0;
			es->light[EARTH].specular[3] = 0;
		}
	break;

	default:
	break;
	}
}

static void
earthPreparePaintScreen (CompScreen *s,
							int		ms)
{
	EARTH_SCREEN (s);

	time_t timer = time (NULL);
	struct tm* currenttime = localtime (&timer);

	struct stat attrib;
	int res;
	char *clouds_filename;

	/* Earth and Sun positions calculations */
	es->dec = 23.4400f * cos((6.2831f/365.0f) *
				((float)currenttime->tm_yday + 10.0f));
	es->gha = (float)currenttime->tm_hour-(es->tz +
				(float)currenttime->tm_isdst) +
				(float)currenttime->tm_sec/3600.0000f;
	
	/* Animation */
            es->rotation += es->rotspeed;
	if (es->rotation > 360)
            es->rotation -= 360;
	else if (es->rotation < -360)
            es->rotation += 360;
	/* Realtime cloudmap */
	res = stat (es->cloudsfile.filename, &attrib);
	if (es->clouds && (es->cloudsthreaddata.started == 0) &&
		((es->clouds_url_changed || (res != 0) ||
			difftime (timer, attrib.st_mtime) > (3600 * 3))) )
	{
		es->cloudsthreaddata.s = s;
		pthread_create (&es->cloudsthreaddata.tid, NULL, DownloadClouds_t, (void*) &es->cloudsthreaddata);
		es->clouds_url_changed = FALSE;
	}

	if (es->cloudsthreaddata.finished == 1)
	{
		pthread_join (es->cloudsthreaddata.tid, NULL);
		clouds_filename = TransformClouds (s);
		if (readImageToTexture (s, es->tex[CLOUDS], clouds_filename, 0, 0))
			compLogMessage ("earth", CompLogLevelInfo,
								"Loaded texture %s", clouds_filename);
		else
			readImageToTexture (s, es->tex[CLOUDS], "clouds.png", 0, 0);
		free(clouds_filename);
		es->cloudsthreaddata.finished = 0;
		es->cloudsthreaddata.started = 0;
	}

	UNWRAP (es, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, ms);
	WRAP (es, s, preparePaintScreen, earthPreparePaintScreen);
}

static void
earthPaintInside (CompScreen			  *s,
					const ScreenPaintAttrib *sAttrib,
					const CompTransform	 *transform,
					CompOutput			  *output,
					int					 size)
{
	EARTH_SCREEN (s);
	CUBE_SCREEN (s);

	ScreenPaintAttrib sA = *sAttrib;

	sA.yRotate += cs->invert * (360.0f / size) *
					(cs->xRotations - (s->x * cs->nOutput));

	CompTransform mT = *transform;

	(*s->applyScreenTransform) (s, &sA, output, &mT);

	glPushMatrix();
	glLoadMatrixf (mT.m);
	glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);
	glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

	/* Pushing all the attribs I'm about to modify*/
	glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT |GL_ENABLE_BIT);

	glEnable (GL_DEPTH_TEST);

	glPushMatrix();

	/* Actual display */
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT1);
	glEnable (GL_BLEND);
	glDisable (GL_COLOR_MATERIAL);

	float ratio = (float)output->height / (float)output->width;
	if (cs->moMode == CUBE_MOMODE_AUTO)
		ratio = (float)s->height / (float)s->width;
	glScalef (ratio * es->earth_size, es->earth_size, ratio * es->earth_size);
	es->previousoutput = output->id;

	/* Animation */
        glRotatef (es->rotation, 0, 1, 0);

	if (cs->moMode == CUBE_MOMODE_MULTI)
		glRotatef ((360.0f / s->nOutputDev) * output->id, 0, 1, 0);

	/* Earth position according to longitude and latitude */
	glRotatef (es->lat - 90, 1, 0, 0);
	glRotatef (es->lon, 0, 0, 1);

	glPushMatrix ();

	/* Sun position (hour * 15 for degree) */
	glRotatef (-es->gha * 15, 0, 0, 1);
	glRotatef (-es->dec, 1, 0, 0);

	glLightfv (GL_LIGHT1, GL_POSITION, es->light[SUN].position);
	glLightfv (GL_LIGHT1, GL_AMBIENT, es->light[SUN].ambient);
	glLightfv (GL_LIGHT1, GL_DIFFUSE, es->light[SUN].diffuse);
	glLightfv (GL_LIGHT1, GL_SPECULAR, es->light[SUN].specular);

	glPopMatrix ();

	/* Earth display */
	glBlendFunc (GL_ONE, GL_ZERO);

	glMaterialfv(GL_FRONT, GL_AMBIENT, es->light[EARTH].ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, es->light[EARTH].diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, es->light[EARTH].specular);
	glMaterialf(GL_FRONT, GL_SHININESS, es->light[EARTH].shininess);

	if (es->shadersupport && es->shaders)
	{
		glUseProgram(es->prog[EARTH]);

		glActiveTexture (GL_TEXTURE0);
		enableTexture (s, es->tex[DAY], COMP_TEXTURE_FILTER_GOOD);

		glActiveTexture (GL_TEXTURE1);
		enableTexture (s, es->tex[NIGHT], COMP_TEXTURE_FILTER_GOOD);

		/* Pass the textures to the shader */
		es->texloc[DAY] = glGetUniformLocation (es->prog[EARTH], "daytex");
		es->texloc[NIGHT] = glGetUniformLocation (es->prog[EARTH], "nighttex");

		glUniform1i (es->texloc[DAY], 0);
		glUniform1i (es->texloc[NIGHT], 1);
	}
	else
		enableTexture (s, es->tex[DAY], COMP_TEXTURE_FILTER_GOOD);

	glCallList (es->list[EARTH]);

	if (es->shadersupport && es->shaders)
	{
		glUseProgram(0);
		disableTexture (s, es->tex[NIGHT]);
		glActiveTexture (GL_TEXTURE0);
		disableTexture (s, es->tex[DAY]);
	}
	else
		disableTexture (s, es->tex[DAY]);

	/* Clouds display */
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMaterialfv(GL_FRONT, GL_SPECULAR, es->light[CLOUDS].specular);
	enableTexture (s, es->tex[CLOUDS], COMP_TEXTURE_FILTER_GOOD);
	glCallList (es->list[CLOUDS]);
	disableTexture (s, es->tex[CLOUDS]);

	glDisable (GL_LIGHT1);

	/* Restore previous state */
	glPopMatrix ();

	glPopAttrib();
	glPopMatrix();

	es->damage = TRUE;

	UNWRAP (es, cs, paintInside);
	(*cs->paintInside) (s, sAttrib, transform, output, size);
	WRAP (es, cs, paintInside, earthPaintInside);
}

static void
earthClearTargetOutput (CompScreen *s,
						float	  xRotate,
						float	  vRotate)
{
	EARTH_SCREEN (s);
	CUBE_SCREEN (s);

	glDisable (GL_LIGHTING);

	glPushMatrix();

	CompOutput* currentoutput = &s->outputDev[(es->previousoutput + 1) % (s->nOutputDev)];

	float ratio = (float)currentoutput->height / (float)currentoutput->width;

	if (cs->moMode == CUBE_MOMODE_ONE)
		ratio = (float) s->height / (float) s->width;

	glScalef (ratio, 1.0f, ratio);

	/* Rotate the skydome according to the mouse and the rotation of the Earth */
	glRotatef (vRotate - 90, 1.0f, 0.0f, 0.0f);
	glRotatef (xRotate + es->rotation, 0.0f, 0.0f, 1.0f);

	if (cs->moMode == CUBE_MOMODE_MULTI)
		glRotatef ((360.0f / s->nOutputDev) * currentoutput->id, 0, 0, 1);

	glRotatef (es->lat, 1, 0, 0);
	glRotatef (es->lon + 180, 0, 0, 1);

	enableTexture (s, es->tex[SKY], COMP_TEXTURE_FILTER_GOOD);
	glCallList (es->list[SKY]);
	disableTexture (s, es->tex[SKY]);

	/* Now rotate to the position of the sun */
	glRotatef (-es->gha * 15, 0, 0, 1);
	glRotatef (es->dec, 1, 0, 0);

	glTranslatef (0, -5, 0);
	glCallList (es->list[SUN]);

	glPopMatrix();

	glEnable (GL_LIGHTING);

	/* Override CubeClearTargetOutput, as it will either draw its own skydome, or blank the screen
	UNWRAP (es, cs, clearTargetOutput);
	(*cs->clearTargetOutput) (s, xRotate, vRotate);
	WRAP (es, cs, clearTargetOutput, earthClearTargetOutput);*/

	glClear (GL_DEPTH_BUFFER_BIT);
}

static void
earthDonePaintScreen (CompScreen * s)
{
	EARTH_SCREEN (s);

	if (es->damage)
	{
		damageScreen (s);
		es->damage = FALSE;
	}

	UNWRAP (es, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP (es, s, donePaintScreen, earthDonePaintScreen);
}

static Bool
earthInitScreen (CompPlugin *p,
		CompScreen *s)
{
	EarthScreen *es;
	char* imagedir;
	char* compizhome;
	struct stat st;

	EARTH_DISPLAY (s->display);
	CUBE_SCREEN (s);

	int i;

	es = malloc (sizeof (EarthScreen));
	if (!es)
		return FALSE;

	s->base.privates[ed->screenPrivateIndex].ptr = es;

	es->damage = FALSE;

	/* Animation */
	es->rotspeed
	es->rotation = 0;


	/* Load texture images */
	for (i = 0; i < 4; i++)
	{
		es->texthreaddata[i].s = s;
		es->texthreaddata[i].num = i;
		LoadTexture_t ((void*) &es->texthreaddata[i]);
	}

	if (!getenv("HOME"))
	{
		compLogMessage ("earth", CompLogLevelWarn,
						"HOME environment variable not set");
		return FALSE;
	}

	assert (asprintf (&imagedir, "%s%s", getenv("HOME"), "/.compiz/images") ==
						strlen (getenv ("HOME")) + strlen ("/.compiz/images"));
	/* Check if directory exists and create it if not.
	 * If path exists but is not a writable directory, the plugin will fail to load */
	if (stat (imagedir, &st) == 0)
	{
		if (!S_ISDIR(st.st_mode))
		{
			compLogMessage ("earth", CompLogLevelWarn,
							"%s exists but is not a directory\n", imagedir);
			free (imagedir);
			return FALSE;
		}
	} else
	{
		compLogMessage ("earth", CompLogLevelInfo,
						"Creating %s/\n", imagedir);
		if (mkdir (imagedir, S_IRWXU) == -1)
		{
			assert (asprintf (&compizhome, "%s%s", getenv("HOME"), "/.compiz") ==
								strlen (getenv ("HOME")) + strlen ("/.compiz"));
			if (mkdir (compizhome, S_IRWXU) == -1 ||
				mkdir (imagedir, S_IRWXU) == -1)
			{
				compLogMessage ("earth", CompLogLevelWarn,
								"Could not create %s\n", imagedir);
				free (compizhome);
				free (imagedir);
				return FALSE;
			}
			free (compizhome);
		}
	}
	free (imagedir);

	/* cloudsfile initialization */
	assert (asprintf (&es->cloudsfile.filename, "%s%s", getenv("HOME"), "/.compiz/images/clouds.jpg") ==
									strlen (getenv("HOME")) + strlen ("/.compiz/images/clouds.jpg"));
	es->cloudsfile.stream = NULL;
	es->cloudsthreaddata.started = 0;
	es->cloudsthreaddata.finished = 0;
	es->cloudsthreaddata.url = NULL;
	es->clouds_url_changed = FALSE;

	/* cURL initialization */
	curl_global_init (CURL_GLOBAL_DEFAULT);
	es->curlhandle = curl_easy_init();

	if (es->curlhandle)
	{
		es->cloudsthreaddata.url = earthGetCloudsUrl (s);
		curl_easy_setopt (es->curlhandle, CURLOPT_URL, earthGetCloudsUrl (s));
		curl_easy_setopt (es->curlhandle, CURLOPT_WRITEFUNCTION, write_clouds_file);
		curl_easy_setopt (es->curlhandle, CURLOPT_WRITEDATA, &es->cloudsfile);
	}

	/* Load the shaders */
	CreateShaders(es);

	/* Lights and materials settings */
	for (i = 0; i < 4; i++)
	{
		es->light[SUN].ambient[i] = 0.2;
		es->light[SUN].diffuse[i] = 1;
		es->light[SUN].specular[i] = 1;
		es->light[SUN].position[i] = 0;

		es->light[EARTH].ambient[i] = 0.1;
		es->light[EARTH].diffuse[i] = 1;
		es->light[EARTH].specular[i] = 0;

		es->light[CLOUDS].specular[i] = 0;
	}
	es->light[SUN].position[1] = 1;

	es->light[EARTH].shininess = 50.0;

	/* Display lists creation */
	CreateLists (es);

	for (i = 0; i < 4; i++)
	{
		if (es->imagedata[i].image)
		{
			imageBufferToTexture (s, es->tex[i], es->imagedata[i].image, es->imagedata[i].width, es->imagedata[i].height);
			free (es->imagedata[i].image);
		}
	}

	/* BCOP */
	earthSetLatitudeNotify (s, earthScreenOptionChanged);
	earthSetLongitudeNotify (s, earthScreenOptionChanged);
	earthSetTimezoneNotify (s, earthScreenOptionChanged);
	earthSetShadersNotify (s, earthScreenOptionChanged);
	earthSetCloudsNotify (s, earthScreenOptionChanged);
	earthSetRotationNotify (s, earthScreenOptionChanged);
	earthSetCloudsUrlNotify (s, earthScreenOptionChanged);
	earthSetEarthSizeNotify (s, earthScreenOptionChanged);

	earthScreenOptionChanged (s, earthGetShadersOption (s), EarthScreenOptionShaders);
	earthScreenOptionChanged (s, earthGetEarthSizeOption (s), EarthScreenOptionEarthSize);

	WRAP (es, s, donePaintScreen, earthDonePaintScreen);
	WRAP (es, s, preparePaintScreen, earthPreparePaintScreen);
	WRAP (es, cs, clearTargetOutput, earthClearTargetOutput);
	WRAP (es, cs, paintInside, earthPaintInside);

	return TRUE;
}

static void
earthFiniScreen (CompPlugin *p,
			 CompScreen *s)
{
	EARTH_SCREEN (s);
	CUBE_SCREEN (s);

	int i;

	/* Free display list */
	glDeleteLists (es->list[0], 4);

	/* Free textures data */
	for (i = 0; i < 4; i++)
		destroyTexture (s, es->tex[i]);

	/* Detach and free shaders */
	DeleteShaders (es);

	/* cURL cleanup */
	if (es->curlhandle)
		curl_easy_cleanup (es->curlhandle);
	curl_global_cleanup ();

	UNWRAP (es, s, donePaintScreen);
	UNWRAP (es, s, preparePaintScreen);
	UNWRAP (es, cs, clearTargetOutput);
	UNWRAP (es, cs, paintInside);

	free (es);
}

static Bool
earthInitDisplay (CompPlugin  *p,
			  CompDisplay *d)
{
	EarthDisplay *ed;

	if (!checkPluginABI ("core", CORE_ABIVERSION) ||
		!checkPluginABI ("cube", CUBE_ABIVERSION))
		return FALSE;

	if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
		return FALSE;

	ed = malloc (sizeof (EarthDisplay));
	if (!ed)
		return FALSE;

	ed->screenPrivateIndex = allocateScreenPrivateIndex (d);
	if (ed->screenPrivateIndex < 0)
	{
		free (ed);
		return FALSE;
	}

	d->base.privates[earthDisplayPrivateIndex].ptr = ed;

	return TRUE;
}

static void
earthFiniDisplay (CompPlugin  *p,
			  CompDisplay *d)
{
	EARTH_DISPLAY (d);

	freeScreenPrivateIndex (d, ed->screenPrivateIndex);
	free (ed);
}

static Bool
earthInit (CompPlugin *p)
{
	earthDisplayPrivateIndex = allocateDisplayPrivateIndex ();

	if (earthDisplayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static CompBool
earthInitObject (CompPlugin *p,
			 CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0,
		(InitPluginObjectProc) earthInitDisplay,
		(InitPluginObjectProc) earthInitScreen
	};

	RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
earthFiniObject (CompPlugin *p,
			 CompObject *o)
{
	static FiniPluginObjectProc dispTab[] = {
		(FiniPluginObjectProc) 0,
		(FiniPluginObjectProc) earthFiniDisplay,
		(FiniPluginObjectProc) earthFiniScreen
	};

	DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static void
earthFini (CompPlugin *p)
{
	freeDisplayPrivateIndex (earthDisplayPrivateIndex);
}

CompPluginVTable earthVTable = {
	"earth",
	0,
	earthInit,
	earthFini,
	earthInitObject,
	earthFiniObject,
	0,
	0
};

CompPluginVTable *
getCompPluginInfo (void)
{
	return &earthVTable;
}
