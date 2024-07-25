#!/usr/bin/env bash

# This script builds an "Actually-Portable Executable" version of Lilt,
# the standalone Lil interpreter, producing a single binary which
# is usable on a wide range of platforms (Windows, OSX, Linux, etc)

set -e

# fetch cosmopolitan dep(s) if needed
if [[ ! -f "./scripts/ape/cosmopolitan.h" ]]; then
	echo "fetching cosmopolitan libc..."
	mkdir -p scripts/ape
	pushd scripts/ape
	wget https://justine.lol/cosmopolitan/cosmopolitan.zip
	unzip cosmopolitan.zip
	popd
fi

# select tooling
COMPILER="gcc"
OBJCOPY="objcopy"
if [[ "$OSTYPE" == "darwin"* ]]; then
	if ! command -v x86_64-elf-gcc &> /dev/null ; then
	    echo -e "ERROR: missing gcc compiler collection. Install via homebrew with:\n  brew install x86_64-elf-gcc"
	    exit
	fi
	COMPILER="x86_64-elf-gcc"
	OBJCOPY="x86_64-elf-objcopy"
fi

# build the sucker
VERSION=$(cat VERSION)
$COMPILER -g -Os -static -nostdlib -nostdinc -fno-pie -no-pie -mno-red-zone \
   -fno-omit-frame-pointer -mno-tls-direct-seg-refs -gdwarf-4 -DVERSION="\"$VERSION\"" \
   -o ./c/build/lilt.com.dbg ./c/lilt.c -fuse-ld=bfd -Wl,-T,scripts/ape/ape.lds -Wl,--gc-sections \
   -include scripts/ape/cosmopolitan.h scripts/ape/crt.o scripts/ape/ape-no-modify-self.o scripts/ape/cosmopolitan.a
$OBJCOPY -S -O binary ./c/build/lilt.com.dbg lilt.com
echo "successfully compiled lilt.com"

# sanity-check against the full test suite
./scripts/test_interpreter.sh "./lilt.com"
./lilt.com tests/dom/arrays.lil
./lilt.com tests/dom/images.lil
./lilt.com tests/dom/domtests.lil
./lilt.com tests/dom/test_roundtrip.lil
./lilt.com tests/puzzles/weeklychallenge.lil

# note: on osx (zsh) it is necessary to invoke the output as:
# bash -c './lilt.com'
