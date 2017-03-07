#!/usr/bin/env perl
use strict;

my $arg = shift;
print q(
static char *xxx[] = {
"10 15 2 1 ",
"0 c None",
"1 c black",
"0000000000",
);
for (0 .. 8) {
    print qq("0${_}0",\n) for unpack "B8", pack "c", ($arg & (1 << $_));
}
print <<EOF;
"1111111110",
"1111111110",
"1111111110",
"1111111110",
"0000000000"
};
EOF

