## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

shadersdir = $(METISSE_SHADERS_DIR)

SHADERS_PROG = blur-f.glsl simple-anim-f.glsl
shaders_DATA = $(SHADERS_PROG) shaders-files-list

EXTRA_DIST = $(shaders_DATA)

CLEANFILES = shaders-files-list

shaders-files-list: Makefile
	list="$(SHADERS_PROG)" ; \
	rm -f shaders-files-list ; \
	for f in $$list ; do \
		echo $$f >> shaders-files-list; \
	done ;
