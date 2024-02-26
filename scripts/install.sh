#!/usr/bin/env bash
# install lilt, decker and lil support resources

DESTDIR=${DESTDIR:=""}
PREFIX=${PREFIX:="/usr/local"}
INSTALLDIR="${DESTDIR}${PREFIX}/bin/"

# install binaries
if [[ ! -f "./c/build/lilt" ]]; then
	echo "skipping lilt. no binary found. build first with 'make lilt'"
fi
if [ -f "./c/build/lilt" ]; then
	echo "copying lilt to ${INSTALLDIR}..."
	mkdir -p "${INSTALLDIR}"
	install -C ./c/build/lilt "${INSTALLDIR}lilt"
fi
if [[ ! -f "./c/build/decker" ]]; then
	echo "skipping decker. no binary found. build first with 'make decker'"
fi
if [ -f "./c/build/decker" ]; then
	echo "copying decker to ${INSTALLDIR}..."
	mkdir -p "${INSTALLDIR}"
	install -C ./c/build/decker "${INSTALLDIR}decker"
fi

# install syntax profiles
SUBLIME=~/Library/Application\ Support/Sublime\ Text/
if [ -d "$SUBLIME" ]; then
	echo "installing lil syntax profile for Sublime Text 4 (mac)..."
	mkdir -p "${SUBLIME}/Packages"
	cp -r syntax/Lil.tmbundle "${SUBLIME}/Packages"
else
	SUBLIME=~/Library/Application\ Support/Sublime\ Text\ 3/
	if [ -d "$SUBLIME" ]; then
		echo "installing lil syntax profile for Sublime Text 3 (mac)..."
		mkdir -p "${SUBLIME}/Packages"
		cp -r syntax/Lil.tmbundle "${SUBLIME}/Packages"
	fi
fi
SUBLIME=~/.config/sublime-text-3/
if [ -d "$SUBLIME" ]; then
	echo "installing lil syntax profile for Sublime Text (linux)..."
	mkdir -p "${SUBLIME}/Packages"
	cp -r syntax/Lil.tmbundle "${SUBLIME}/Packages"
fi

VIM=~/.vim/
if [ -d "$VIM" ]; then
	echo "installing lil syntax profile for vim in ${VIM}..."
	mkdir -p "${VIM}ftdetect"
	mkdir -p "${VIM}syntax"
	cp syntax/vim/ftdetect/lil.vim "${VIM}ftdetect/lil.vim"
	cp syntax/vim/syntax/lil.vim   "${VIM}syntax/lil.vim"
fi

echo "done."
