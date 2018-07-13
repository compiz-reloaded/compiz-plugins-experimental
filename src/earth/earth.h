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

#ifndef __EARTH_H__
#define __EARTH_H__

#define _GNU_SOURCE

#include <math.h>
#include <float.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <assert.h>
#include <curl/curl.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <compiz-core.h>
#include <compiz-cube.h>

extern int earthDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;

#define GET_EARTH_DISPLAY(d) \
    ((EarthDisplay *) (d)->base.privates[earthDisplayPrivateIndex].ptr)
#define EARTH_DISPLAY(d) \
    EarthDisplay *ed = GET_EARTH_DISPLAY(d);

#define GET_EARTH_SCREEN(s, ed) \
    ((EarthScreen *) (s)->base.privates[(ed)->screenPrivateIndex].ptr)
#define EARTH_SCREEN(s) \
    EarthScreen *es = GET_EARTH_SCREEN(s, GET_EARTH_DISPLAY(s->display))

#define DAY	0
#define NIGHT	1
#define CLOUDS	2
#define SKY	3

#define EARTH	0
#define SUN	1


typedef struct _LightParam
{
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat position[4];
    GLfloat emission[4];
    GLfloat shininess;
} LightParam;

typedef struct _texthreaddata
{
    CompScreen* s;
    int num;
    pthread_t tid;
} TexThreadData;

typedef struct _cloudsthreaddata
{
    CompScreen* s;
    pthread_t tid;
    int started;
    int finished;
    char *url;
} CloudsThreadData;

typedef struct _imagedata
{
    void* image;
    int width, height;
} ImageData;

typedef struct _cloudsFile
{
    char* filename;
    FILE* stream;
} CloudsFile;

typedef struct _EarthDisplay
{
    int screenPrivateIndex;
} EarthDisplay;

typedef struct _EarthScreen
{
    DonePaintScreenProc    donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;

    /* Config parameters */
    float lon, lat;
    float tz;
    Bool  shaders;
    Bool  clouds;
    float rotspeed;
    Bool  clouds_url_changed;
    float earth_size;

    int previousoutput;

    /* Sun position */
    float dec, gha;

    /* Animation */
    float rotation;

    /* Threads */
    TexThreadData texthreaddata [4];
    CloudsThreadData cloudsthreaddata;

    /* Clouds */
    CURL* curlhandle;
    CloudsFile cloudsfile;

    /* Textures */
    ImageData imagedata [4];
    CompTexture* tex [4];

    /* Rendering */
    LightParam light [3];
    GLuint list [4];

    /* Shaders */
    GLboolean shadersupport;
    GLuint vert;
    GLuint frag;
    GLuint prog;

} EarthScreen;

#endif
