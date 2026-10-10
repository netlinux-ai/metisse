# Phase 1 compositor. Needs: libx11, libxcomposite, libxdamage, libxfixes, libgl
CC      ?= cc
CFLAGS  ?= -O2 -g -Wall -Wextra
PKGS    := x11 xcomposite xdamage xfixes gl
CFLAGS  += $(shell pkg-config --cflags $(PKGS))
LDLIBS  += $(shell pkg-config --libs $(PKGS)) -lm

composite: composite.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f composite
.PHONY: clean
