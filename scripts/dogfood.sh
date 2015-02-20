#!/usr/bin/env bash
# Usage: dogfood.sh tempfile-stem path-to-tas file.tas[.cpp] [ files... ]
if [[ $V == 1 ]] ; then set -x ; fi
here=`dirname $BASH_SOURCE`
stem=`basename $1`
tas=$2
shift
shift
base=`mktemp -t $stem`

function fail ()
{
	fmt=$1
	file=$2
	bn=$(basename $file)
	echo $bn: FAILED
	mkdir -p dogfood_failures/$bn
	cp -p $file $base.$fmt.{en,de}.? dogfood_failures/$bn/
}

function en ()
{
	fmt=$1
	instance=$2
	$tas -f $fmt - | tee $base.$fmt.en.$instance
}

function de ()
{
	fmt=$1
	instance=$2
	$tas -d -f $fmt - | tee $base.$fmt.de.$instance
}

function check ()
{
	fmt=$1
	typ=$2
	diff -q $base.$fmt.$typ.$3 $base.$fmt.$typ.$4
}

function cycle ()
{
	fmt=$1
	en $fmt 1 | de $fmt 1 | en $fmt 2 | de $fmt 2 | en $fmt 3 | de $fmt 3 > /dev/null
}

function match ()
{
	fmt=$1
	typ=$2
	file=$3
	check $fmt $typ 1 2 && check $fmt $typ 2 3 && echo $(basename $file) @ $fmt: OK || fail $fmt $file
}

# TODO support obj when identical objects can be made reliably
for fmt in memh raw text ; do
	for file in $* ; do
		trap "rm $base.$fmt.{en,de}.[0123]" EXIT
		if [[ $file = *.cpp ]] ; then
			pp="cpp -I$here/../lib"
		else
			pp=cat
		fi
		$pp $file | cycle $fmt
		match $fmt en $file
	done

	base=random
	file=$base
	$here/random.sh | tee $base | de text 0 | cycle $fmt
	match $fmt en $file
done

