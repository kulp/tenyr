#!/usr/bin/env bash
count=1024
hexdump -n$count -e'"%08x\n"' /dev/urandom | \
    while read in ; do
        f=$(cut -c1 <<<$in)
        z=$(cut -c2 <<<$in)
        x=$(cut -c3 <<<$in)
        y=$(cut -c4 <<<$in)
        o=$(cut -c5 <<<$in)
        i=$(cut -c6- <<<$in)

        if [[ $((RANDOM & 3)) == 0 ]]
            then let "z = 0" ; fi
        if [[ $((RANDOM & 3)) == 0 ]]
            then let "x = 0" ; fi
        if [[ $((RANDOM & 3)) == 0 ]]
            then let "y = 0" ; fi
        if [[ $((RANDOM & 3)) == 0 ]]
            then let "o = 0" ; fi
        echo -n 0x$f$z$x$y$o
        if [[ $((RANDOM & 1)) == 0 ]]
            then echo 000 ; else echo $i ; fi
    done

