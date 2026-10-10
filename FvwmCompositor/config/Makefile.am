## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

SUBDIRS = colours window themes FvwmScript

configdir = $(METISSE_FVWM_CONFIG_DIR)

CONFIG_FILES = metisserc applications background bindings bindings-functions \
	desks fonts menus menustyle panels shading stroke styles demos

config_DATA = scripting.pl config-files-list config $(CONFIG_FILES)

EXTRA_DIST = scripting.pl config-files-list $(CONFIG_FILES)

CLEANFILES = config metisserc config-files-list

config-files-list: Makefile $(SUBDIRS)
	list="$(CONFIG_FILES)" ; \
	rm -f config-files-list ; \
	for f in $$list ; do \
		echo $$f >> config-files-list; \
	done ; \
	if [ "$(SUBDIRS)" != "" ]; then \
        subdirs="$(SUBDIRS)" ; \
        for d in $$subdirs ; do \
		if [ -f $$d/config-files-list ]; then \
			cat $$d/config-files-list >> config-files-list ; \
		fi ; \
	  done ; \
	fi
