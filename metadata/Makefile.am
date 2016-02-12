xmldir       = $(metadatadir)

if ANIMATIONPLUS_PLUGIN
#depends on animationaddon
animationplusxml = animationplus.xml.in
endif

if ANIMATIONSIM_PLUGIN
#depends on animation
animationsimxml = animationsim.xml.in
endif

if PHOTOWHEEL_PLUGIN
#depends on cube
photowheelxml = photo.xml.in
endif

if ATLANTIS_PLUGIN
#depends on cube
atlantisxml = atlantis.xml.in
endif

if CUBEMODEL_PLUGIN
#depends on cube
cubemodelxml = cubemodel.xml.in
endif

if SNOWGLOBE_PLUGIN
#depends on cube
snowglobexml = snowglobe.xml.in
endif

if GHOST_PLUGIN
#depends on regex
#depends on mousepoll, for some reason not listed metadata file
ghostxml = ghost.xml.in
endif

if WIZARD_PLUGIN
#depends on mousepoll
wizardxml = wizard.xml.in
endif

#dialog depends on regex

if SCREENSAVER_PLUGIN
#depends on text and xscreensaver (external)
screensaverxml = screensaver.xml.in
endif

if FREEWINS_PLUGIN
#depends on cairo-xlib (external)
freewinsxml = freewins.xml.in
endif

if ELEMENTS_PLUGIN
#depends on text
elementsxml = elements.xml.in
endif

if STACKSWITCH_PLUGIN
#depends on text
stackswitchxml = stackswitch.xml.in
endif

if WORKSPACENAMES_PLUGIN
#depends on text
workspacenamesxml = workspacenames.xml.in
endif

xml_in_files =               \
	anaglyph.xml.in       \
	dialog.xml.in         \
	fakeargb.xml.in       \
	fireflies.xml.in      \
	mswitch.xml.in        \
	putplus.xml.in        \
	smartput.xml.in       \
	snow.xml.in           \
	star.xml.in           \
	static.xml.in         \
	swap.xml.in           \
	throw.xml.in          \
	tile.xml.in           \
	toggledeco.xml.in     \
	$(animationplusxml)   \
	$(animationsimxml)    \
	$(atlantisxml)        \
	$(cubemodelxml)       \
	$(elementsxml)        \
	$(freewinsxml)        \
	$(ghostxml)           \
	$(photowheelxml)      \
	$(screensaverxml)     \
	$(snowglobexml)       \
	$(stackswitchxml)     \
	$(wizardxml)          \
	$(workspacenamesxml)
	
xml_files    = $(xml_in_files:.xml.in=.xml)
xml_DATA     = $(xml_files)

@INTLTOOL_XML_RULE@

EXTRA_DIST =		\
	$(xml_in_files)

DISTCLEANFILES =	\
	$(xml_files)

