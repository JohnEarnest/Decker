#!/usr/bin/env sh
# bundle the HTML/JS of the web-decker implementation as a c header file,
# for inclusion with the native versions of decker and lilt.

set -e

DECK=$1
if test -z "$DECK" ; then
	echo "Missing source deck">&2
	exit 1
fi

DST=c/resources.h

printf "%s\n" "// auto-generated from web-decker source!" > $DST
xxd -i js/lil.js      >> $DST
xxd -i js/danger.js   >> $DST
xxd -i js/decker.html >> $DST
xxd -i js/decker.js   >> $DST
xxd -i $DECK          >> $DST
