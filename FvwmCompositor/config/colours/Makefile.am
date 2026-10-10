## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

configdir = $(METISSE_FVWM_CONFIG_DIR)/colours

CONFIG_FILES = Ia_Ora_blue.theme \
	Ia_Ora_free.theme \
	Ia_Ora_gray.theme \
	Ia_Ora_orange.theme \
	Metisse_Contrast.theme \
	Metisse.theme \
	Opale.theme

config_DATA = $(CONFIG_FILES)

EXTRA_DIST = $(CONFIG_FILES)

noinst_SCRIPTS = config-files-list

config-files-list: Makefile
	list="$(CONFIG_FILES)"; \
	rm -f config-files-list ; \
	for f in $$list ; do \
		echo colours/$$f >> config-files-list; \
	done ;

CLEANFILES = config-files-list
