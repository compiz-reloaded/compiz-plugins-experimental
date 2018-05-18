iconsdir = $(pluginiconsdir)/scalable/apps
icons_DATA =			\
	plugin-earth.svg	\
	plugin-throw.svg

install-data-hook:
	$(shell which gtk-update-icon-cache &> /dev/null &&	\
	gtk-update-icon-cache -f -t $(pluginiconsdir))
