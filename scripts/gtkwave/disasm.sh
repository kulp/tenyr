#!/bin/bash
here=$(dirname $0)
tas="$here/../../tas"
while read b ; do
    if [[ $b == "" ]]; then
        echo "EMPTY"
    else
        # TODO rewrite to avoid having to start tas for every new line.
        # Currently we do this to avoid buffering problems.
        out=$($tas -d -f text - <<<"$b")
        if [[ $? == 0 ]]; then
            echo "$out" | cut -d'#' -f1 | tr -s ' ' | sed 's/^[[:space:]]*//'
        else
            echo "BAD"
        fi
    fi
done
