#!/bin/bash
pmccabe ${1:-src/*.c} |
    while read comp x x x x file func ; do
        if [ "$comp" -gt ${2:-10} ] ; then
            echo $comp $file $func
        fi
    done |
    sort -gr |
	column -t
