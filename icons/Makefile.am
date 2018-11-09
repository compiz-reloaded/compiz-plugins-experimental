iconsdir = $(pluginiconsdir)/scalable/apps
icons_DATA =				\
	plugin-anaglyph.svg		\
	plugin-animationplus.svg	\
	plugin-animationsim.svg		\
	plugin-atlantis.svg		\
	plugin-cubemodel.svg		\
	plugin-dialog.svg		\
	plugin-earth.svg		\
	plugin-elements.svg		\
	plugin-fakeargb.svg		\
	plugin-fireflies.svg		\
	plugin-freewins.svg		\
	plugin-ghost.svg		\
	plugin-mswitch.svg		\
	plugin-photo.svg		\
	plugin-putplus.svg		\
	plugin-screensaver.svg		\
	plugin-smartput.svg		\
	plugin-snowglobe.svg		\
	plugin-snow.svg			\
	plugin-stackswitch.svg		\
	plugin-star.svg			\
	plugin-static.svg		\
	plugin-swap.svg			\
	plugin-throw.svg		\
	plugin-tile.svg			\
	plugin-toggledeco.svg		\
	plugin-wizard.svg		\
	plugin-workspacenames.svg

EXTRA_DIST = \
	$(icons_DATA)

install-data-hook:
	if test x$(DESTDIR) = x; then \
		$(UPDATE_ICON_CACHE) -f -t $(pluginiconsdir); \
	fi

uninstall-local:
	if test x$(DESTDIR) = x; then \
		rm $(pluginiconsdir)/icon-theme.cache; \
	fi
