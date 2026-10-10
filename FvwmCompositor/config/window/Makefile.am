## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

configdir = $(METISSE_FVWM_CONFIG_DIR)/window

CONFIG_FILES =	Devel.theme Ia_Ora.theme Metisse.theme \
	Redmond.theme Vectors.theme

config_DATA = $(CONFIG_FILES)

EXTRA_DIST = $(CONFIG_FILES)

noinst_SCRIPTS = config-files-list

config-files-list: Makefile
	list="$(CONFIG_FILES)" ; \
	rm -f config-files-list ; \
	for f in $$list ; do \
		echo window/$$f >> config-files-list; \
	done ;

CLEANFILES = config-files-list
