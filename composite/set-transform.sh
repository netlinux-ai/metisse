#!/bin/bash
# Usage: set-transform.sh WINDOW_ID RX RY RZ [SCALE [OPACITY]]
# Sets _METISSE_TRANSFORM on the top-level (frame) window containing WINDOW_ID.
# Angles are in degrees; scale and opacity are 0..1 factors (defaults 1 and 1).
# With only WINDOW_ID given, removes the transform. Uses $DISPLAY.
id=$1; [ -n "$id" ] || { sed -n '2,5p' "$0"; exit 2; }
# climb to the child of the root window
root=$(xwininfo -root | awk '/Window id:/{print $4; exit}')
while :; do
	parent=$(xwininfo -id "$id" -tree | awk '/Parent window id:/{print $4; exit}')
	[ -z "$parent" ] || [ "$parent" = "$root" ] && break
	id=$parent
done
if [ $# -eq 1 ]; then xprop -id "$id" -remove _METISSE_TRANSFORM; exit; fi
m() { awk -v v="$1" 'BEGIN{printf "%d", v*1000}'; }
xprop -id "$id" -format _METISSE_TRANSFORM 32i \
	-set _METISSE_TRANSFORM "$(m "$2"), $(m "$3"), $(m "$4"), $(m "${5:-1}"), $(m "${6:-1}")"
