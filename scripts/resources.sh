#!/usr/bin/env bash
# bundle the HTML/JS of the web-decker implementation as a c header file,
# for inclusion with the native versions of decker and lilt.

set -e

DECK=${1?"Missing source deck"}
DST=c/resources.h

echo -e "// auto-generated from web-decker source!\n" > $DST
xxd -i js/lil.js      >> $DST
xxd -i js/danger.js   >> $DST
xxd -i js/decker.html >> $DST
xxd -i js/decker.js   >> $DST
xxd -i $DECK          >> $DST
