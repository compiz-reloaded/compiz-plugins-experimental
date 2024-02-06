#include "snowglobe-internal.h"
#include "snowglobe_options.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>


void
SnowflakeTransform (snowflakeRec * snow)
{

    glTranslatef (snow->y, snow->z, snow->x);
    glRotatef (-snow->psi, 0.0, 1.0, 0.0);
    glRotatef (snow->theta, 1.0, 0.0, 0.0);
}

void
newSnowflakePosition(SnowglobeScreen *as, int i)
{
    int sector = NRAND(as->hsize);
    float ang = randf(as->arcAngle*toRadians)-0.5*as->arcAngle*toRadians;
    float r = (as->radius-0.01*as->snow[i].size/2);
    float factor = sinf(0.5*(PI-as->arcAngle*toRadians))/
		   sinf(0.5*(PI-as->arcAngle*toRadians)+fabsf(ang));
    ang += (0.5+((float) sector))*as->arcAngle*toRadians;
    ang = fmodf(ang,2*PI);

    float d = randf(1);
    d=(1-d*d)*r*factor;

    as->snow[i].x = d*cosf(ang);
    as->snow[i].y = d*sinf(ang);
    as->snow[i].z = 0.5;
}

void
SnowflakeDrift (CompScreen *s, int index)
{
    float progress;

    SNOWGLOBE_SCREEN (s);
    CUBE_SCREEN (s);

    //float oldXRotate = as->xRotate;
    //float oldVRotate = as->vRotate;

    (*cs->getRotation) (s, &(as->xRotate), &(as->vRotate), &progress);

    as->xRotate = fmodf( as->xRotate-cs->invert * (360.0f / s->hsize) *
			 ((s->x* cs->nOutput)), 360 );

    snowflakeRec * snow = &(as->snow[index]);

    float speed = snow->speed*as->speedFactor;
    speed/=1000;
    float x = snow->x;
    float y = snow->y;
    float z = snow->z;

    float sideways = 2*(randf(2*speed)-speed);
    float vertical = -speed;

    if (snowglobeGetShakeCube(s))
    {
	x+= sideways*cosf(as->xRotate*toRadians)*cosf(as->vRotate*toRadians)
	   -vertical*cosf(as->xRotate*toRadians)*sinf(as->vRotate*toRadians);

	y+= sideways*sinf(as->xRotate*toRadians)*cosf(as->vRotate*toRadians)
	   +vertical*sinf(as->xRotate*toRadians)*sinf(as->vRotate*toRadians);

	z+= sideways*sinf(as->vRotate*toRadians)
	   +vertical*cosf(as->vRotate*toRadians);
    }
    else
    {
	x+=sideways;
	y+=sideways;
	z+=vertical;
    }


    float bottom = (snowglobeGetShowGround(s) ? getHeight(as->ground, x, y) : -0.5)+0.01*snow->size/2;

    if (z<bottom)
    {
	z = 0.5;
	newSnowflakePosition(as, index);

	x = snow->x;
	y = snow->y;
    }

    float top = 0.5-0.01*snow->size/2;
    if (z>top)
    {
	z = top;
    }


    float ang = atan2f(y, x);

    int i;
    for (i=0; i< as->hsize; i++)
    {
	float cosAng = cosf(fmodf(i*as->arcAngle*toRadians-ang, 2*PI));
	if (cosAng<=0)
	    continue;

	float r = hypotf(x, y);
	float d = r*cosAng-(as->distance-0.01*snow->size/2);

	if (d>0)
	{
	    x -= d*cosf(ang)*fabsf(cosf(i*as->arcAngle*toRadians));
	    y -= d*sinf(ang)*fabsf(sinf(i*as->arcAngle*toRadians));
	}
    }

    snow->x = x;
    snow->y = y;
    snow->z = z;

    snow->psi = fmodf(snow->psi+snow->dpsi*as->speedFactor, 360);
    snow->theta= fmodf(snow->theta+snow->dtheta*as->speedFactor, 360);
}
