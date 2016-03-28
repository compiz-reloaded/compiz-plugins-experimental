#ifndef _CUBEMODEL_INTERNAL_H
#define _CUBEMODEL_INTERNAL_H

#include <math.h>
#include <float.h>

#include <compiz-core.h>
#include <compiz-cube.h>

#define LRAND()   ((long) (random () & 0x7fffffff))
#define NRAND(n)  ((int) (LRAND () % (n)))
#define MAXRAND   (2147483648.0) /* unsigned 1 << 31 as a float */

/* some constants */
#define PI        3.14159265358979323846
#define PIdiv2    1.57079632679489661923
#define toDegrees 57.2957795130823208768
#define toRadians 0.01745329251994329577

/* return random number in range [0,x) */
#define randf(x) ((float) (rand () / (((double) RAND_MAX + 1) / (x))))

extern int cubemodelDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;

#define GET_CUBEMODEL_DISPLAY(d) \
    ((CubemodelDisplay *) (d)->base.privates[cubemodelDisplayPrivateIndex].ptr)
#define CUBEMODEL_DISPLAY(d) \
    CubemodelDisplay *cmd = GET_CUBEMODEL_DISPLAY(d);

#define GET_CUBEMODEL_SCREEN(s, cmd) \
    ((CubemodelScreen *) (s)->base.privates[(cmd)->screenPrivateIndex].ptr)
#define CUBEMODEL_SCREEN(s) \
    CubemodelScreen *cms = GET_CUBEMODEL_SCREEN(s, GET_CUBEMODEL_DISPLAY(s->display))

typedef struct _vect3d
{
    float r[3];
} vect3d;

typedef struct _vect2d
{
    float r[2];
} vect2d;

typedef struct _groupIndices
{
    int polyCount;
    int complexity;

    int startV;
    int numV;
    int startT;
    int numT;
    int startN;
    int numN;

    int materialIndex; //negative for no new material

    Bool texture;
    Bool normal;
} groupIndices;

typedef struct _mtlStruct
{
    char *name;

    GLfloat Ka[4];
    GLfloat Kd[4];
    GLfloat Ks[4];

    GLfloat Ns[1]; /* shininess */
    GLfloat Ni[1]; /* optical_density - currently not used*/

    int illum;

    unsigned height, width;

    int map_Ka; /* index to tex, negative for none */
    int map_Kd;
    int map_Ks;
    int map_d;

    int map_params; /* index tex map with height/width */
} mtlStruct;

typedef struct _CubemodelObject
{
    pthread_t thread;
    Bool      threadRunning;
    Bool      finishedLoading;
    Bool      updateAttributes; /* after finished loading in thread, read in
				   new attributes not read in before to avoid
				   race condition*/
    char *filename;
    char *post;

    int size;
    int lenBaseFilename;
    int startFileNum;
    int maxNumZeros;

    GLuint dList;
    Bool   compiledDList;

    float  rotate[4], translate[3], scale[3];
    float  rotateSpeed, scaleGlobal;
    float  color[4];

    int   fileCounter;
    Bool  animation;
    int   fps;
    float time;

    vect3d **reorderedVertex;
    vect2d **reorderedTexture;
    vect3d **reorderedNormal;

    unsigned int *indices;
    groupIndices *group;

    vect3d *reorderedVertexBuffer;
    vect2d *reorderedTextureBuffer;
    vect3d *reorderedNormalBuffer;

    int nVertex;
    int nTexture;
    int nNormal;
    int nGroups;
    int nIndices;
    int nUniqueIndices;
    int *nMaterial;

    mtlStruct    **material;
    CompTexture  *tex; /* stores all the textures */
    char         **texName;
    unsigned int *texWidth;
    unsigned int *texHeight;

    int nTex;
} CubemodelObject;

typedef struct _fileParser
{
    FILE *fp;
    char *oldStrline;
    char *buf;
    int  bufferSize;
    int  cp;
    Bool lastTokenOnLine;
    int  tokenCount;
} fileParser;

typedef struct _CubemodelDisplay
{
    int screenPrivateIndex;
} CubemodelDisplay;

typedef struct _CubemodelScreen
{
    DonePaintScreenProc    donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;

    int   hsize;
    float sideDistance; /* perpendicular distance to side wall from centre */
    float topDistance;  /* perpendicular distance to top wall from centre */
    float radius;       /* radius on which the hSize points lie */
    float arcAngle;   	/* 360 degrees / horizontal size */
    float ratio;        /* screen width to height */

    GLuint objDisplayList;

    CubemodelObject **models;
    char            **modelFilename;
    int             numModels;
} CubemodelScreen;

Bool
cubemodelDrawModelObject (CompScreen      *s,
			  CubemodelObject *data,
			  float           scale);

Bool
cubemodelDeleteModelObject (CompScreen      *s,
			    CubemodelObject *data);

Bool
cubemodelModifyModelObject (CubemodelObject *data,
			    CompOption      *option,
			    int             nOption);

Bool
cubemodelUpdateModelObject (CompScreen      *s,
			    CubemodelObject *data,
			    float           time);

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
			 float           fps);

Bool
cubemodelDrawVBOModel (CompScreen      *s,
		       CubemodelObject *data,
		       float           *vertex,
		       float           *normal);

fileParser *
initFileParser (FILE *fp,
                int bufferSize);

void
updateFileParser (fileParser *fParser,
                  FILE *fp);

void
freeFileParser (fileParser *);

char *
getLine (fileParser *);

char *
getLineToken (fileParser *);

char *
getLineToken2 (fileParser *, Bool);

void
skipLine (fileParser *);

char *
strsep2 (char **strPtr, const char *delim);

#endif
