#!/bin/bash
# uninstall lilt, lilac, and lil support resources

DESTDIR=""
PREFIX="/usr/local"
INSTALLDIR="${DESTDIR}${PREFIX}/bin/"

sudo rm -f "${INSTALLDIR}lilt"
sudo rm -f "${INSTALLDIR}lilac"

rm -rf ~/Library/Application\ Support/Sublime\ Text/Packages/Lil.tmbundle
rm -rf ~/Library/Application\ Support/Sublime\ Text\ 3/Packages/Lil.tmbundle
rm -rf ~/.config/sublime-text-3/Lil.tmbundle

rm -f ~/.vim/ftdetect/lil.vim
rm -f ~/.vim/syntax/lil.vim

echo "done."
