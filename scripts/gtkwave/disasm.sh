#!/bin/bash
here=$(dirname $0)
tas="$(make -s -C $here/../.. showbuilddir)/tas"
# We have to run a loop even in the error condition, to keep gtkwave from hanging
[[ -x $tas ]] || { while read ; do echo "tas not found" ; done ; exit 1; }
while true ; do
	$tas --strict --quiet --disassemble --format text - | tr -u -s ' ' | sed -le 's/^ //; s/0x0*\([0-9][0-9]*\)/0x\1/'
	echo BAD
done
