#!/usr/bin/env bash
# Invoke this script with two parameters:
#   version (e.g. "2.1.3") of GNU Lightning to install
#   prefix for installation
set -xeuo pipefail

version=$1
out=$2
path=$(mktemp -d)
curl -s "http://ftp.gnu.org/gnu/lightning/lightning-$version.tar.gz" | tar -C "$path" -zx
cd "$path/lightning-$version"
./configure --prefix="$out"
make -j4
make install
