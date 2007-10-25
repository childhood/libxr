" Vim syntax file
" Language:     XDL
" Maintainer:   Ondrej Jirman <megous@megous.com>
" Last Change:  2007 Oct 25

" Quit when a (custom) syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn include @cTop syntax/c.vim
syn sync clear
unlet b:current_syntax

" A bunch of useful DM keywords
syn keyword xdlKeyword          struct namespace error servlet array take
syn keyword xdlType             string int double boolean time blob any
syn match   xdlOperator         contained "[<>=]"
syn match   xdlNumber           contained "\<\d\+"
syn region  xdlComment          start="/\*"  end="\*/"
syn region  xdlCCode            start="<%"  end="%>" contains=@cTop
syn match   xdlLineComment      "//.*"

hi def link xdlComment          Comment
hi def link xdlLineComment      Comment
hi def link xdlType             Type
hi def link xdlKeyword          Keyword
hi def link xdlNumber           Number
hi def link xdlOperator         Operator

let b:current_syntax = "xdl"

" vim: ts=8
