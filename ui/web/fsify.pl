#!/usr/bin/perl -p
# Insert a line into a generated script to give a reference to FS when it otherwise would be inaccessible
BEGIN{ $count = 0; }

if (/preInit/ && !$count++) {
	print <<EOF;
Module['_FSref'] = FS;
EOF
}

