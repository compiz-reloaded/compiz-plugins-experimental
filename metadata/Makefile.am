xmldir       = $(metadatadir)

xml_in_files =               \
	anaglyph.xml.in       \
	animationplus.xml.in  \
	animationsim.xml.in   \
	atlantis.xml.in       \
	cubemodel.xml.in      \
	dialog.xml.in         \
	elements.xml.in       \
	fakeargb.xml.in       \
	fireflies.xml.in      \
	freewins.xml.in       \
	ghost.xml.in          \
	mswitch.xml.in        \
	photo.xml.in          \
	putplus.xml.in        \
	screensaver.xml.in    \
	smartput.xml.in       \
	snow.xml.in           \
	snowglobe.xml.in      \
	stackswitch.xml.in    \
	star.xml.in           \
	static.xml.in         \
	swap.xml.in           \
	throw.xml.in          \
	tile.xml.in           \
	toggledeco.xml.in     \
	wizard.xml.in         \
	workspacenames.xml.in
	
xml_files    = $(xml_in_files:.xml.in=.xml)
xml_DATA     = $(xml_files)

@INTLTOOL_XML_RULE@

EXTRA_DIST =		\
	$(xml_in_files)

DISTCLEANFILES =	\
	$(xml_files)

