#!/usr/bin/env perl
use strict;

my %off;

open my $index, "index.memb"
    or die "no index available";

chomp, $off{$_} = $. - 1 while <$index>;
chomp, print((reverse split //, unpack("b5", pack "C", $off{$_})), "\n") while <>;
