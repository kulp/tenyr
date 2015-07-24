#!/bin/bash
icarus=$1
need_major=0
need_minor=10
if [[ -x $icarus ]] ; then
    version=$($icarus -V | sed -nEe '/.*Icarus Verilog version ([0-9.]+).*/{s//\1/;p;}')
    $MAKESTEP -n "version $version found at $icarus"
    major=${version%%.*}
    minpatch=${version#$major.}
    minor=${minpatch%.*}
    patch=${minpatch#$minor.}
    if [[ $major -gt $need_major ]] || [[ $major -eq $need_major && $minor -ge $need_minor ]] ; then
        $MAKESTEP
        exit 0
    else
        $MAKESTEP " -- too old"
        exit 1
    fi
fi
