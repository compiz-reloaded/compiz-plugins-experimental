#include "flyingwindows.h"


#define NE 0
#define NO 1
#define SE 2
#define SO 3
#define C 4

void DisplayFlyingWindows::handleEvent( XEvent *event )
{
	DisplayEffect::handleEvent( event );
	if( event->type == MapNotify )
	{
		CompWindow* w = findWindowAtDisplay( d, event->xmap.window );
		if(w)
			WindowFlyingWindows::getInstance(w).initWindow();
	}
}

bool ScreenFlyingWindows::enable()
{
	ss->angleCam = 0.0;
	ss->screenCenter = Vector( 0.0, screensaverGetBounce(s->display) ? 0.2 : 0.0,\
								-screensaverGetAttractionDepth(s->display) );
	ss->camera = Matrix::identity;
	ss->desktopOpacity = OPAQUE;

	for( CompWindow* w = s->windows; w; w=w->next )
		WindowFlyingWindows::getInstance(w).initWindow();

	return ScreenEffect::enable();
}

void ScreenFlyingWindows::disable()
{
	for( CompWindow* w = s->windows; w; w=w->next )
	{
		WindowFlyingWindows& sw = WindowFlyingWindows::getInstance(w);
		if( sw.active )
			sw.transformFadeOut = ss->cameraMat * sw.transform;

		else sw.opacityFadeOut = sw.opacity;
	}
	ss->cameraMat = Matrix::identity;
	ScreenEffect::disable();
}

void ScreenFlyingWindows::addForce( const Point& p1, const Point& p2, const Point& center, Vector& resultante, Vector& couple, float w, Bool attract )
{
	Vector u = p2 - p1;
	float d = u.norm();
	u.normalize();

	if( d < 1e-5 ) d = 1e-5;

	Vector force = attract ? w*u*d*d : -w*u/(d*d);
	resultante += force;

	couple += ( center - p1 ) ^ force;
}

void ScreenFlyingWindows::preparePaintScreen( int msSinceLastPaint )
{
	ScreenEffect::preparePaintScreen( msSinceLastPaint );

	float ratio = screensaverGetAttractionRepulsionRatio(s->display)/100.0;
	float wAt = ratio;
	float wRt = 1-ratio;

	SCREENSAVER_DISPLAY(s->display);

	if( sd->state.fadingIn )
	{
		wAt *= getProgress();
		wRt *= getProgress();

		ss->desktopOpacity = (GLushort)( OPAQUE * ( 1.0 - getProgress() ));
	}

	if( sd->state.fadingOut )
	{
		ss->desktopOpacity = (GLushort)(OPAQUE * getProgress());
	}

	if( !sd->state.fadingOut )
	{
		ss->angleCam += ((float)msSinceLastPaint)/100000.0;
		if( ss->angleCam > 0.03 ) ss->angleCam=0.03;

		ss->camera.rotate( ss->angleCam*msSinceLastPaint, 0.0, 1.0, 0.0 );

		Matrix centerTrans = Matrix::identity;
		Matrix centerTransInv = Matrix::identity;

		Vector screenCenterTranslated = ss->screenCenter.toCoordsSpace(s);
		screenCenterTranslated[z] *= s->width;

		centerTrans.scale( 1.0, 1.0, 1.0 / s->width);
		centerTrans.translate( screenCenterTranslated );
		centerTransInv.translate( -screenCenterTranslated );
		centerTransInv.scale( 1.0, 1.0, 1.0 * s->width);

		ss->cameraMat = centerTrans * ss->camera * centerTransInv;
	}

	for ( CompWindow* w = s->windows; w; w = w->next)
	{
		WindowFlyingWindows& sw = WindowFlyingWindows::getInstance(w);

		if( sw.active )
		{
			if( sd->state.fadingOut )
				sw.transform = interpolate( sw.transformFadeOut, Matrix::identity, getProgress() );

			else
			{
				int collisionVertex = -1;
				int collisionIteration = 1;
				Vector a, p, o, omega;
				do
				{
					Vector resultante, couple, resultanteA, coupleA;
					resultante = couple = resultanteA = coupleA = Vector::null;
					Vector windowcenter = sw.vertex[C];
					float mass = sqrt(((float)(WIN_W(w)*WIN_H(w)))/(w->screen->width*w->screen->height));

					float wR = 1e-8/mass*wRt;
					float wA = 1e-8/mass*wAt;

					int numPoint = 0;
					for( int i = 0; i<5; i++)
					{
						for( CompWindow* w2 = w->screen->windows; w2; w2=w2->next )
						{
							WindowFlyingWindows& sw2 = WindowFlyingWindows::getInstance(w2);
							if( w2 != w && sw2.active )
							{
								numPoint++;
								for( int j = 0; j<5; j++)
									addForce( sw.vertex[i],  sw2.vertex[j], windowcenter, resultante, couple, wR, FALSE );
							}
						}
						addForce( sw.vertex[i], ss->screenCenter, windowcenter, resultanteA, coupleA, wA, TRUE );
					}

					if( numPoint < 1 )
						numPoint = 1;

					resultante += resultanteA*numPoint;
					couple += coupleA*numPoint;

					if( collisionVertex != -1 )
					{
						float wb = sw.vertex[collisionVertex][y]/msSinceLastPaint*5e-4;
						resultante[y] = -wb;
						float tmp = couple[z];
						couple = (sw.vertex[collisionVertex]-windowcenter) ^ Vector( 0.0, -wb, 0.0 );
						couple[z] = tmp;
						sw.speed[y] = 0;
					}

					a = resultante - 5e-4*mass*sw.speed;
					omega = couple*1000 - 5e-3*mass*sw.speedrot;

					p = msSinceLastPaint*msSinceLastPaint*a+msSinceLastPaint*sw.speed;
					sw.speed += msSinceLastPaint*a;

					o = msSinceLastPaint*msSinceLastPaint*omega+msSinceLastPaint*sw.speedrot;
					sw.speedrot += msSinceLastPaint*omega;

					sw.transformTrans.translate( p[x]*s->width, -p[y]*s->height, p[z] );
					sw.transformRot.rotate( o.norm(), o );

					sw.transform = sw.transformTrans * sw.centerTrans * sw.transformRot * sw.centerTransInv;

					sw.recalcVertices();

					if ( screensaverGetBounce(s->display) )
					{
						for( int i = 0; i < 4; i++ )
							if( sw.vertex[i][y] < -0.5 )
								collisionVertex = i;
					}

				} while ( collisionVertex != -1 && collisionIteration-- );
			}
		}
		else
		{
			if( sd->state.fadingOut )
			{
				sw.opacity = (GLushort)( sw.opacityOld*getProgress() + sw.opacityFadeOut*(1-getProgress()) );
			}
			else
			sw.steps = (int)( (msSinceLastPaint * OPAQUE) / (screensaverGetFadeInDuration(s->display)*1000.0) );
		}
	}
}

void ScreenFlyingWindows::donePaintScreen()
{
	damageScreen(s);
	ScreenEffect::donePaintScreen();
}

void ScreenFlyingWindows::paintScreen (CompOutput   *outputs,
				       int          numOutputs,
				       unsigned int mask)
{
    outputs = &s->fullscreenOutput;
    numOutputs = 1;

    ScreenEffect::paintScreen (outputs, numOutputs, mask);
}

Bool ScreenFlyingWindows::paintOutput(	const ScreenPaintAttrib *sAttrib, \
										const CompTransform* transform, Region region, \
										CompOutput* output, unsigned int mask)
{
	clearTargetOutput (s->display, GL_COLOR_BUFFER_BIT);
	return ScreenEffect::paintOutput( sAttrib, transform, region, output, mask | PAINT_SCREEN_TRANSFORMED_MASK );
}

void ScreenFlyingWindows::paintTransformedOutput(	const ScreenPaintAttrib* sAttrib, \
													const CompTransform* transform, Region region, \
													CompOutput *output, unsigned int mask )
{
	Bool wasCulled = glIsEnabled (GL_CULL_FACE);
	if (wasCulled)
		glDisable (GL_CULL_FACE);

	int oldFilter = s->display->textureFilter;
	if( screensaverGetMipmaps(s->display) )
		s->display->textureFilter = GL_LINEAR_MIPMAP_LINEAR;


	mask &= ~PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

	GLboolean bTwoSite;
	glGetBooleanv(GL_LIGHT_MODEL_TWO_SIDE, &bTwoSite );
	glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, s->lighting);

	ScreenEffect::paintTransformedOutput ( sAttrib, transform, &s->region,
								output, mask );

	glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, bTwoSite);

	s->filter[SCREEN_TRANS_FILTER] = oldFilter;
	s->display->textureFilter = oldFilter;

	if (wasCulled)
		glEnable (GL_CULL_FACE);
}

WindowFlyingWindows::WindowFlyingWindows( CompWindow* w ) :
	WindowEffect(w),
	active( FALSE ),
	opacity( w->paint.opacity ),
	opacityFadeOut( 0 ),
	opacityOld( 0 ),
	steps(0)
{

}

// Returns true if w is a flying window
bool WindowFlyingWindows::isActiveWin()
{
	return	!w->attrib.override_redirect && \
			w->mapNum && \
			w->attrib.map_state == IsViewable && \
			!( w->wmType & ( CompWindowTypeDockMask | CompWindowTypeDesktopMask )) && \
		/*	!( w->state & ( CompWindowStateSkipPagerMask | CompWindowStateShadedMask )) && \*/
			matchEval (screensaverGetWindowMatch (w->screen->display), w);
}

// Initialize window transformation matrices and vertices
void WindowFlyingWindows::initWindow()
{
	CompScreen* s = this->w->screen;

	active = isActiveWin();
	if( active )
	{
		float x = WIN_X(this->w);
		float y = WIN_Y(this->w);
		float w = WIN_W(this->w);
		float h = WIN_H(this->w);

		transform = transformRot = transformTrans = Matrix::identity;
		centerTrans = centerTransInv = Matrix::identity;

		centerTrans.scale( 1.0, 1.0, 1.0/s->width );
		centerTrans.translate( x + w/2.0, y + h/2.0, 0.0 );
		centerTransInv.translate( -(x + w/2.0), -(y + h/2.0), 0.0);
		centerTransInv.scale( 1.0f, 1.0f, 1.0f*s->width );

		recalcVertices();
		speed = speedrot = Vector::null;
	}
	else opacityOld = opacity;
}

// Update window vertices
void WindowFlyingWindows::recalcVertices()
{
	float x = WIN_X(this->w);
	float y = WIN_Y(this->w);
	float w = WIN_W(this->w);
	float h = WIN_H(this->w);

	vertex[NO] = Point( x, y, 0.0 );
	vertex[NE] = Point( x + w, y, 0.0 );
	vertex[SO] = Point( x, y + h, 0.0 );
	vertex[SE] = Point( x + w, y + h, 0.0 );
	vertex[C] = Point( x + w/2.0, y + h/2.0, 0.0 );

	// Apply the window transformation and normalize
	for( int i = 0; i < 5; i++ )
		vertex[i] = ( transform * vertex[i] ).toScreenSpace(this->w->screen);
}

Bool WindowFlyingWindows::paintWindow(	const WindowPaintAttrib* attrib, \
										const CompTransform* transform, Region region, unsigned int mask)
{
	WindowPaintAttrib sAttrib = *attrib;
	Matrix wTransform;

	if( !active )
	{
		SCREENSAVER_DISPLAY(w->screen->display);

		if( opacity && steps && !sd->state.fadingOut )
		{
			if( opacity < steps )
				opacity = 0;
			else opacity -= steps;
			steps = 0;
		}

		sAttrib.opacity = opacity;
		wTransform = transform;
	}
	else
	{
		SCREENSAVER_SCREEN( w->screen );
		wTransform = transform * ss->cameraMat * this->transform;
		mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		if( w->state & CompWindowStateSkipPagerMask )
			return WindowEffect::paintWindow( attrib, (CompTransform*)wTransform.m, region, mask );
	}

	if (w->alpha || sAttrib.opacity != OPAQUE)
		mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	// from paint.c
	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
	{
		if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
			return FALSE;
		if (mask & PAINT_WINDOW_NO_CORE_INSTANCE_MASK)
			return FALSE;
		if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
			return FALSE;
		if (w->shaded)
			return FALSE;
		return TRUE;
	}

	FragmentAttrib fragment;
	initFragmentAttrib (&fragment, &sAttrib);

	glPushMatrix();
	glLoadMatrixf( wTransform.m );
	bool status = w->screen->drawWindow( w, (CompTransform*)wTransform.m, &fragment, region, mask );
	glPopMatrix();
	return status;
}
