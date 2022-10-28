#!/bin/bash
# build the browser version of decker

set -e

SRC=${1?"Missing source deck"}
DST=${2?"Missing destination"}
VERSION=${3?"Missing version number"}

echo -e "<body><script language=\"decker\">\n" > $DST
cat $SRC >> $DST
echo -e "</script>\n" >> $DST
cat js/decker.html >> $DST
echo -e "<script>\n" >> $DST
echo -e "VERSION=\"${VERSION}\"\n" >> $DST
cat js/lil.js >> $DST
cat js/decker.js >> $DST
echo -e "</script></body>" >> $DST

echo "built web-decker"
