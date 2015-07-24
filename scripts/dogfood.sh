#!/usr/bin/env bash
# Usage: dogfood.sh tempfile-stem path-to-tas file.tas[.cpp] [ files... ]
set -o pipefail
if [[ $V == 1 ]] ; then ECHO=echo ; else ECHO=true ; fi
here=`dirname $BASH_SOURCE`
stem=`basename $1`
tas=$2
shift
shift
base=`mktemp -t $stem`
pids=()

function fail ()
{
    fmt=$1
    file=$2
    bn=$(basename $file)
    echo $bn: FAILED
    mkdir -p dogfood_failures/$bn
    cp -p $file $base.$fmt.{en,de}.*.? dogfood_failures/$bn/
    exit 1
}

function en ()
{
    fmt=$1
    instance=$2
    $tas $flags -f $fmt - | tee $base.$fmt.en.$flags.$instance
    if [[ $? != 0 ]] ; then return 1 ; fi
}

function de ()
{
    fmt=$1
    instance=$2
    $tas $flags -d -f $fmt - | tee $base.$fmt.de.$flags.$instance
    if [[ $? != 0 ]] ; then return 1; fi
}

function check ()
{
    fmt=$1
    typ=$2
    diff -q $base.$fmt.$typ.$flags.$3 $base.$fmt.$typ.$flags.$4
}

function cycle ()
{
    fmt=$1
    en $fmt 1 | de $fmt 1 | en $fmt 2 | de $fmt 2 | en $fmt 3 | de $fmt 3 > /dev/null
    if [[ $? != 0 ]] ; then return 1 ; fi
}

function match ()
{
    fmt=$1
    typ=$2
    file=$3
    check $fmt $typ 1 2 && check $fmt $typ 2 3 && $ECHO "$(basename $file) @ $fmt ($flags) : OK" || fail $fmt $file
}

# TODO support obj when identical objects can be made reliably
for flags in -v "" ; do
    for fmt in memh raw text ; do
        for file in $* ; do
            trap "rm $base.$fmt.{en,de}.[0123]" EXIT
            if [[ $file = *.cpp ]] ; then
                pp="cpp -I$here/../lib"
            else
                pp=cat
            fi
            $pp $file | cycle $fmt
            if [[ $? != 0 ]] ; then fail $fmt $file ; fi
            match $fmt en $file
        done & pids+=( $! )
        trap "kill $! 2>/dev/null" EXIT

        ( base=random
        file=$base
        $here/random.sh | tee $base | de text 0 | cycle $fmt
        if [[ $? != 0 ]] ; then fail $fmt $file ; fi
        match $fmt en $file ) & pids+=( $! )
        trap "kill $! 2>/dev/null" EXIT
    done
done

for pid in ${pids[@]} ; do
    wait $pid || exit $?
done

