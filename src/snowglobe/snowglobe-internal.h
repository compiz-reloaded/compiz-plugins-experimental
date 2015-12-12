#ifndef _SNOWGLOBE_INTERNAL_H
#define _SNOWGLOBE_INTERNAL_H

#define LRAND()                 ((long) (random() & 0x7fffffff))
#define NRAND(n)                ((int) (LRAND() % (n)))
#define MAXRAND                 (2147483648.0) /* unsigned 1<<31 as a float */


#include <math.h>
#include <float.h>

/* some constants */
#define PI     M_PI
#define PIdiv2 M_PI_2
#define toDegrees (180.0f * M_1_PI)
#define toRadians (M_PI / 180.0f)

//return random number in range [0,x)
#define randf(x) ((float) (rand()/(((double)RAND_MAX + 1)/(x))))


#include <compiz-core.h>
#include <compiz-cube.h>

extern int snowglobeDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;

#define GET_SNOWGLOBE_DISPLAY(d) \
    ((SnowglobeDisplay *) (d)->base.privates[snowglobeDisplayPrivateIndex].ptr)
#define SNOWGLOBE_DISPLAY(d) \
    SnowglobeDisplay *ad = GET_SNOWGLOBE_DISPLAY(d);

#define GET_SNOWGLOBE_SCREEN(s, ad) \
    ((SnowglobeScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)
#define SNOWGLOBE_SCREEN(s) \
    SnowglobeScreen *as = GET_SNOWGLOBE_SCREEN(s, GET_SNOWGLOBE_DISPLAY(s->display))


typedef struct _snowflakeRec
{
    float x, y, z;
    float theta, psi;
    float dpsi, dtheta;
    float speed, size;
}
snowflakeRec;

typedef struct _Vertex
{
    float v[3];
    float n[3];
}
Vertex;

typedef struct _Water
{
    int      size;
    float    distance;
    int      sDiv;

    float  bh;
    float  wa;
    float  swa;
    float  wf;
    float  swf;

    Vertex       *vertices;
    unsigned int *indices;

    unsigned int nVertices;
    unsigned int nIndices;

    unsigned int nSVer;
    unsigned int nSIdx;
    unsigned int nWVer;
    unsigned int nWIdx;

    float    wave1;
    float    wave2;
}
Water;

typedef struct _SnowglobeDisplay
{
    int screenPrivateIndex;
}
SnowglobeDisplay;

typedef struct _SnowglobeScreen
{
    DonePaintScreenProc donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;

    int numSnowflakes;

    snowflakeRec *snow;

    Water *water;
    Water *ground;

    float xRotate;
    float vRotate;

    float waterHeight; //water surface height

    int hsize;
    float distance;    //perpendicular distance to wall from centre
    float radius;      //radius on which the hSize points lie
    float arcAngle;    //360 degrees / horizontal size

    float speedFactor; // multiply snowflake speeds by this value

    GLuint snowflakeDisplayList;
}
SnowglobeScreen;

void
updateWater (CompScreen *s, float time);

void
updateGround (CompScreen *s, float time);

void
updateHeight (Water *w);

void
freeWater (Water *w);

void
drawWater (Water *w, Bool full, Bool wire);

void
drawGround (Water *w, Water *g);

void
drawBottomGround (int size, float distance, float bottom);

Bool
isInside (CompScreen *s, float x, float y, float z);

float
getHeight (Water *w, float x, float z);



void SnowflakeTransform(snowflakeRec *);
void newSnowflakePosition(SnowglobeScreen *, int);
void SnowflakeDrift (CompScreen *, int);

void initializeWorldVariables(CompScreen *);
void updateSpeedFactor(float);
void RenderWater(int, float, Bool, Bool);

void DrawSnowflake (int);
void initDrawSnowflake(void);
void finDrawSnowflake(void);

void DrawSnowman (int);


//All calculations that matter with angles are done clockwise from top.
//I think of it as x=radius, y=0 being the top (towards 1st desktop from above view)
//and the z coordinate as height.

#endif
