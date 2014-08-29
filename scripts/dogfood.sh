#!/bin/bash
here=`dirname $BASH_SOURCE`
stem=`basename $1`
shift
tas=$(make BUILDDIR= -C $here/.. showbuilddir)/tas
base=`mktemp -t $stem`
for file in $* ; do
	temp1=$base.1.tas
	temp2=$base.2.tas
	temp3=$base.3.tas
	trap "rm $temp1 $temp2 $temp3" EXIT
	if [[ $file = *.cpp ]] ; then
		pp="cpp -I$here/../lib"
	else
		pp=cat
	fi
	$pp $file | $tas - | $tas -d -v - | tee $temp1 | $tas - | $tas -d -v - | tee $temp2 | $tas - | $tas -d -v -o $temp3 -
	diff -q $temp1 $temp2 && diff -q $temp2 $temp3 && echo $(basename $file): OK || echo $(basename $file): FAILED
done
