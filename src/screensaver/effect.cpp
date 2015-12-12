#include "effect.h"


#define sigmoid(x) (1.0f/(1.0f+exp(-5.5f*2*((x)-0.5))))
#define sigmoidProgress(x) ((sigmoid(x) - sigmoid(0)) / \
							(sigmoid(1) - sigmoid(0)))

DisplayEffect::DisplayEffect( CompDisplay* w ) :
	DisplayWrapper(w),
	cleanEffect(false),
	loadEffect(false)
{}

bool ScreenEffect::enable()
{
	progress = 0.0;
	return true;
}

void ScreenEffect::preparePaintScreen( int msSinceLastPaint )
{
	SCREENSAVER_DISPLAY( s->display );
	if( sd->state.running )
	{
		if( sd->state.fadingIn )
		{
			float fadeDuration = screensaverGetFadeInDuration(s->display)*1000.0;
			progress = sigmoidProgress(((float)ss->time)/fadeDuration);
			ss->time += msSinceLastPaint;
			if( ss->time >= fadeDuration )
			{
				if( screensaverGetStartAutomatically(s->display) )
					XActivateScreenSaver(s->display->display);
				sd->state.fadingIn = FALSE;
				ss->time = 0;
			}
		}

		else if( sd->state.fadingOut )
		{
			float fadeDuration = screensaverGetFadeOutDuration(s->display)*1000.0;
			progress = sigmoidProgress( ((float)ss->time)/fadeDuration );
			ss->time += msSinceLastPaint;
			if( ss->time >= fadeDuration )
			{
				clean();
				sd->effect->cleanEffect = true;
				sd->state.running = FALSE;
				damageScreen(s);
			}
		}

		else progress = 1.0;
	}

	ScreenWrapper::preparePaintScreen( msSinceLastPaint );
}
