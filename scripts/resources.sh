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
./c/build/ra js/lil.js      >> $DST
./c/build/ra js/danger.js   >> $DST
./c/build/ra js/decker.html >> $DST
./c/build/ra js/decker.js   >> $DST
./c/build/ra $DECK          >> $DST
