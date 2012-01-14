#!/bin/bash
here=$(dirname $0)
git ls-files --no-empty-directory --exclude-standard | perl -lne 'chomp; print unless -d or m#/gen/#' | cloc --list-file=/dev/stdin $*
