" Vim syntax file
" Language: tenyr asm
" Maintainer: Darren Kulp <darren@kulp.ch>
" Latest Revision: 10 Aug 2020

"if exists("b:current_syntax")
"  finish
"endif

syn keyword tenyrRegister A B C D E F G H I J K L M N O P
syn keyword tenyrRegister a b c d e f g h i j k l m n o p

syn match tenyrDelim '[][]'

"syn keyword tenyrArrow <- ->
syn match tenyrOp '\(^\|[[:space:]]\)\@<=\([-<>^.+\&|*\~]\|<<\|<<<\|>>\|>>>\|!=\|==\|>=\|<=\||\~\|&\~\|\^\^\)\([[:space:]]\|$\)\@='
syn match tenyrArrow '<-\|->'

syn match tenyrDirective '\.\(global\|word\|set\|zero\)\>'
syn match tenyrStrDir '\.chars'

syn keyword tenyrTodo illegal
syn keyword tenyrTodo contained TODO FIXME XXX NOTE
syn region tenyrComment start='#' end='$' contains=tenyrTodo

syn match tenyrLabel '\w\+:'
syn match tenyrLocal '\.L\w\+:'

syn region tenyrChar start="'" end="'" contains=tenyrEscape
syn match tenyrEscape contained '\\[\\'0bfnrtv]'
syn region tenyrString start='"' end='"' contained contains=tenyrEscape
syn region tenyrStrRegion start="\.chars\>" end="$" keepend contains=tenyrString,tenyrStrDir

syn match tenyrNumber '-\?\(0\|[1-9]\([0-9_]*[0-9]\)\?\)\>' " decimal
syn match tenyrNumber '-\?0[0-7_]*[0-7]' " octal
syn match tenyrNumber '-\?0x[[:xdigit:]_]*[[:xdigit:]]' " hexadecimal
syn match tenyrNumber '-\?0b[01_]*[01]' " binary

let b:current_syntax = "tenyr"

hi def link tenyrTodo        Todo
hi def link tenyrComment     Comment
hi def link tenyrRegister    Statement
hi def link tenyrOp          Operator
hi def link tenyrString      String
hi def link tenyrChar        String
hi def link cDefine          Define
hi def link cInclude         Include
hi def link cIncluded        String
hi def link tenyrNumber      Number
hi def link tenyrDirective   Statement
hi def link tenyrStrDir      Statement
hi def link tenyrDelim       Delimiter
hi def link tenyrLabel       Label
hi def link tenyrLocal       Label
hi def link tenyrArrow       SpecialChar

