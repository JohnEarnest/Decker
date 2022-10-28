" Vim syntax file
" Language:   Lil (Decker)
" Filenames:  *.lil
" Maintainer: John Earnest

if exists("b:current_syntax")
  finish
endif

syn match     lilComment "#.*$"
syn match     lilEsc     contained /\\[n"\\]/
syn region    lilStr     start=/"/ end=/"/ skip=/\\"/ contains=lilEsc

syn match     lilName    contained /[_?a-zA-Z][_?a-zA-Z0-9]*/
syn keyword   lilOn      on nextgroup=lilName skipwhite

syn keyword   lilControl if else end while each send do select extract update insert into from where by orderby asc desc with
syn keyword   lilPrims   floor cos sin tan exp ln sqrt count sum min max raze first last range list table rows cols mag unit heading
syn keyword   lilPrims   split fuse cat dict take drop in at join cross parse format typeof unless flip limit

hi def link lilComment Comment
hi def link lilName    Function
hi def link lilEsc     SpecialChar
hi def link lilStr     String
hi def link lilOn      Statement
hi def link lilControl Repeat
hi def link lilPrims   Operator
