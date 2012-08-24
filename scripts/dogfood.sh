#!/bin/bash
here=`dirname $0`
stem=`basename $1`
tas=$here/../tas
base=`mktemp -t $stem`
temp1=$base.1.tas
temp2=$base.2.tas
temp3=$base.3.tas
pp="cpp -I$here/../lib"
$pp $1 | $tas - | $tas -d -v - | tee $temp1 | $tas - | $tas -d -v - | tee $temp2 | $tas - | $tas -d -v -o $temp3 -
diff -u $temp1 $temp2
diff -u $temp2 $temp3
