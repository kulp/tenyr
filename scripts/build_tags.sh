#!/bin/bash
numtags=1
tags=$(git tag --list 'v*.*.*' | tail -n$numtags)
for t in $tags ; do
    make clobber
    git checkout $t
    if git describe --tags --exact-match ; then
        make upload
    fi
done

