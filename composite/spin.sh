#!/bin/bash
# Usage: spin.sh WINDOW_ID [SECONDS]
# Demo: swings a window through 3D by animating _METISSE_TRANSFORM, then
# removes the transform. Uses $DISPLAY. Window is tilted about Y and X only,
# so remember pointer input is not remapped (clicks hit the flat window).
id=$1; secs=${2:-15}; [ -n "$id" ] || { sed -n '2,5p' "$0"; exit 2; }
root=$(xwininfo -root | awk '/Window id:/{print $4; exit}')
while :; do
	parent=$(xwininfo -id "$id" -tree | awk '/Parent window id:/{print $4; exit}')
	[ -z "$parent" ] || [ "$parent" = "$root" ] && break
	id=$parent
done
trap 'xprop -id "$id" -remove _METISSE_TRANSFORM; exit' INT TERM
fps=25
awk -v n=$((secs * fps)) -v fps=$fps 'BEGIN{
	for (i = 0; i < n; i++) { t = i / fps; k = 6.2831853;
		printf "%d, %d, %d, 1000, 1000\n", 20000*sin(k*t/4), 60000*sin(k*t/6), 8000*sin(k*t/5) } }' |
while read -r v; do
	xprop -id "$id" -format _METISSE_TRANSFORM 32i -set _METISSE_TRANSFORM "$v"
	sleep 0.02
done
xprop -id "$id" -remove _METISSE_TRANSFORM
