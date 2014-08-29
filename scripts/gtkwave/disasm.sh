#!/bin/bash
here=$(dirname $0)
tas="$(make -C $here/../.. showbuilddir)/tas"
while read b ; do
    if [[ $b == "" ]]; then
        echo "EMPTY"
    else
        # TODO rewrite to avoid having to start tas for every new line.
        # Currently we do this to avoid buffering problems.
        out=$($tas --quiet --disassemble --format text - <<<$b | tr -s ' ' | sed 's/^ //')
        if [[ $? == 0 ]]; then
            echo "$out"
        else
            echo "BAD"
        fi
    fi
done
