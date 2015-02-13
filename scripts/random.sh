#!/usr/bin/env bash
count=1024
str=$(LC_ALL=C tr -dc "[:xdigit:]" < /dev/urandom | dd conv=lcase | head -c$((count * 8)))
for i in `seq 0 $((count - 1))`; do
    r=$RANDOM
    f=${str:$((i * 8 + 0)):1}
    z=${str:$((i * 8 + 1)):1}
    x=${str:$((i * 8 + 2)):1}
    y=${str:$((i * 8 + 3)):1}
    o=${str:$((i * 8 + 4)):1}
    i=${str:$((i * 8 + 5)):3}

    if [[ $((r & (3 << 0))) == 0 ]]
        then let "z = 0" ; fi
    if [[ $((r & (3 << 2))) == 0 ]]
        then let "x = 0" ; fi
    if [[ $((r & (3 << 4))) == 0 ]]
        then let "y = 0" ; fi
    if [[ $((r & (3 << 6))) == 0 ]]
        then let "o = 0" ; fi
    echo -n 0x$f$z$x$y$o
    if [[ $((r & (1 << 8))) == 0 ]]
        then echo 000 ; else echo $i ; fi
done

