xmldir       = $(metadatadir)
xml_in_files =		  \
	atlantis.xml.in   \
	cubemodel.xml.in  \
	elements.xml.in   \
	fakeargb.xml.in	  \
	mswitch.xml.in    \
	snow.xml.in       \
	tile.xml.in
	
xml_files    = $(xml_in_files:.xml.in=.xml)
xml_DATA     = $(xml_files)

@INTLTOOL_XML_RULE@

EXTRA_DIST =		\
	$(xml_in_files)

DISTCLEANFILES =	\
	$(xml_files)

