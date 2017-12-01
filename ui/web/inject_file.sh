#!/bin/sh
# Takes three arguments : search token, injected contents, and destination file
# Newline is necessary to delimit filename
sed -i.bak "/$1/r "$2"
" "$3" && rm "$3.bak"
