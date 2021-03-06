" Vim syntax file
" Language:    mu
" Maintainer:  Kartik Agaram <mu@akkartik.com>
" URL:         http://github.com/akkartik/mu
" License:     public domain
"
" Copy this into your ftplugin directory, and add the following to your vimrc
" or to .vim/ftdetect/mu.vim:
"   autocmd BufReadPost,BufNewFile *.mu set filetype=mu

let s:save_cpo = &cpo
set cpo&vim

" todo: why does this periodically lose syntax, like on file reload?
"   $ vim x.mu
"   :e
"? if exists("b:syntax")
"?   finish
"? endif
"? let b:syntax = "mu"

setlocal iskeyword=@,48-57,?,!,_,$,-
setlocal formatoptions-=t  " Mu programs have long lines
setlocal formatoptions+=c  " but comments should still wrap

syntax match muComment /#.*$/  | highlight link muComment Comment
syntax match muSalientComment /##.*$/  | highlight link muSalientComment SalientComment
set comments+=n:#
syntax match muCommentedCode "#? .*"  | highlight link muCommentedCode CommentedCode
let b:cmt_head = "#? "

syntax match muDelimiter "[{}]"  | highlight link muDelimiter Delimiter

" Mu literals
syntax match muLiteral %\<-\?[0-9][0-9A-Fa-f]*\>%
syntax match muLiteral %\<-\?0x[0-9A-Fa-f]\+\>%
syntax match muLiteral %"[^"]*"%
highlight link muLiteral Constant

" sources of action at a distance
syntax match muAssign "<-"
highlight link muAssign SpecialChar

" common keywords
syntax match muControl "^return\>\| return\>"
syntax match muControl "\<jump\>\|\<jump-if[^ ]*"
syntax match muControl "\<break\>\|\<break-if[^ ]*"
syntax match muControl "\<loop\>\|\<loop-if[^ ]*"
highlight link muControl PreProc

syntax match muKeyword " -> "
syntax keyword muKeyword fn type var
highlight link muKeyword PreProc

syntax match muFunction "\(fn\s*\)\@<=\(\S\+\)"
highlight muFunction cterm=underline ctermfg=130

syntax match muTest "\(fn\s*\)\@<=\(test-\S\+\)"
highlight muTest ctermfg=64

syntax match muData "^type\>"
syntax match muData "\<eax\>\|\<ecx\>\|\<edx\>\|\<ebx\>\|\<esi\>\|\<edi\>"
highlight link muData Constant

let &cpo = s:save_cpo
