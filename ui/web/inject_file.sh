#!/bin/sh
# Takes (2 * N) + 1 arguments : (search token, injected contents) * N, plus
# destination file
# Newline is necessary to delimit filename
dest=${!#}
while [ $# -gt 1 ]
do
    sed -i.bak "/$1/r $2
" "$dest" && rm "$dest.bak"
    shift
    shift
done
