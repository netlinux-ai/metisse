#!/bin/bash
# Start the original Metisse: Xmetisse on :1, FVWM, and FvwmCompositor drawing
# into a nested Xephyr on :3. Never touches your real desktop (:0).
# Usage: run-metisse.sh [start|stop|status]      (default: start)
# Run apps in it with: DISPLAY=:1 xterm &
# environment (PATH etc. for a local install prefix); override with METISSE_ENV
source "${METISSE_ENV:-/Data/metisse/build-env.sh}"
HOST=:3 METISSE=:1 SIZE=960x540
CFG=${METISSE_CFG:-/Data/metisse/fvwm-config}   # created on first run
RUN=${XDG_RUNTIME_DIR:-/tmp}/metisse-run; mkdir -p "$RUN"

up() { [ -S /tmp/.X11-unix/X${1#:} ]; }
launch() {  # launch NAME cmd...  (own session, PID recorded; only these are ever killed)
	local n=$1; shift
	setsid "$@" >"$RUN/$n.log" 2>&1 < /dev/null & echo $! > "$RUN/$n.pid"
}

case ${1:-start} in
start)
	if ! up $HOST; then
		launch xephyr Xephyr $HOST -glamor -screen $SIZE -nolisten tcp -title Xephyr-metisse
		sleep 2
	fi
	if ! up $METISSE; then
		launch xmetisse Xmetisse -geometry $SIZE -ac $METISSE
		sleep 3
	fi
	DISPLAY=$HOST launch fvwm metisse-start-fvwm -c "$CFG"
	sleep 10
	echo "Metisse running: view on $HOST, apps use DISPLAY=$METISSE. Logs in $RUN"
	;;
stop)
	# only PIDs this script started
	for n in fvwm xmetisse xephyr; do
		[ -f "$RUN/$n.pid" ] && kill -- -"$(cat "$RUN/$n.pid")" 2>/dev/null
		rm -f "$RUN/$n.pid"
	done
	;;
status)
	for n in xephyr xmetisse fvwm; do
		p=$(cat "$RUN/$n.pid" 2>/dev/null)
		if [ -n "$p" ] && kill -0 "$p" 2>/dev/null; then echo "$n: running ($p)"; else echo "$n: not started by this script"; fi
	done
	up $HOST && echo "$HOST present"; up $METISSE && echo "$METISSE present"; true
	;;
*) echo "usage: $0 [start|stop|status]"; exit 2 ;;
esac
