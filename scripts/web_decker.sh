#!/usr/bin/env bash
# build the browser version of decker

set -e

SRC=${1?"Missing source deck"}
DST=${2?"Missing destination"}
VERSION=${3?"Missing version number"}
DANGER=${4:-0}

echo -e "<meta charset=\"UTF-8\"><body><script language=\"decker\">\n" > $DST
cat $SRC >> $DST
echo -e "</script>\n" >> $DST
cat js/decker.html >> $DST
echo -e "<script>\n" >> $DST
echo -e "VERSION=\"${VERSION}\"\n" >> $DST
echo -e "DANGEROUS=${DANGER}\n" >> $DST
cat js/lil.js >> $DST
cat js/danger.js >> $DST
cat js/decker.js >> $DST
echo -e "</script></body>" >> $DST

echo "built web-decker: ${DST}"
