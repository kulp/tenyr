#!/usr/bin/env perl
use strict;

open my $index, "index.memb"
    or die "no index available";

my @lines = <$index>;

chomp, print $lines[unpack "C", pack "b5", join "", reverse split //, $_] while <>;
