#!/bin/bash
here=$(dirname $0)
tas="$(make -C $here/../.. showbuilddir)/tas"
# We have to run a loop even in the error condition, to keep gtkwave from hanging
[[ -x $tas ]] || { while read ; do echo "tas not found" ; done ; exit 1; }
while true ; do
	$tas --quiet --disassemble --format text - | tr -u -s ' ' | sed -le 's/^ //; s/0x0*/0x/'
	echo BAD
done
