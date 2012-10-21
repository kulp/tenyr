#!/usr/bin/env perl
use strict;
use warnings;

use Net::SSH qw(sshopen2);
use Net::SCP qw(scp);

my $user = $ENV{USER};
my $cflags = "-Os -fomit-frame-pointer -std=c99 -DTEST";

my %hosts = (
        #ia64        => [ { host => "gcc66" , cflags => "" } ],
        #mips64      => [ { host => "gcc49" , cflags => "-march=mips64" } ],
        #sparc64     => [ { host => "gcc54" , cflags => "-m64" } ],
        #hppa        => [ { host => "gcc61" , cflags => "" } ],
        #powerpc64   => [ { host => "gcc110", cflags => "-m64 -mcpu=power7" } ],
        mips32      => [ { host => "gcc49" , cflags => "-march=mips32" } ],
        sparc32     => [ { host => "gcc54" , cflags => "-m32" } ],
        'x86-64'    => [ { host => "gcc12" , cflags => "-m64" } ],
        i686        => [ { host => "gcc12" , cflags => "-m32 -march=i686" } ],
        powerpc32   => [ { host => "gcc110", cflags => "-m32 -mcpu=G3" } ],
    );

print "Host\tArch\tSource\tFunc\tInsns\tBytes\n";
for my $source (@ARGV) {
    for my $arch (keys %hosts) {
        my @recs = @{ $hosts{$arch} };
        for my $rec (@recs) {
            my $host = $rec->{host};
            sshopen2("$user\@$host", *READER, *WRITER, "bash");
            print WRITER "mktemp -d\n";
            chomp(my $tmpdir = <READER>);
            die "No tempdir" unless $tmpdir;
            scp($source, "$user\@$host:$tmpdir/");
            print WRITER "cd $tmpdir\n";
            (my $object = $source) =~ s/\.c$/.o/;
            print WRITER "gcc -c -o $object $cflags $rec->{cflags} $source\n";
            # Power7 binaries show up entry points as ' D ' instead of ' T '
            print WRITER "nm -g $object | grep ' [TD] ' > syms\n";
            print WRITER "wc -l syms\n";
            my $count = (split /\s+/, scalar <READER>)[0];
            print WRITER "cat syms\n";
            my @symbols = map { scalar <READER> } 1 .. $count;
            my $funcs = join "|", map { (split /\s+/)[2] } @symbols;
            print WRITER "objdump -d $object\n";
            print WRITER "rm -rf $tmpdir\n";
            close WRITER;
            open my $fh, ">", "$source.$arch.$host.disasm.txt";
            open my $slim, ">", "$source.$arch.$host.disasm.slim.txt";
            while (<READER>) {
                print $fh $_;
                if (/($funcs)>:/../^$/ and $1) {
                    my $func = $1;
                    next unless /:/;
                    print $slim $_;
                    my $r = $rec->{$source}{funcs}{$func} ||= +{ };
                    # adjust for one extra line
                    $r->{instructions} //= -1;
                    $r->{instructions}++;
                    my @fields = split /\t/;
                    if (@fields > 1) {
                        my @bytes = $fields[1] =~ /([[:xdigit:]]{2})\s*/g;
                        $r->{bytes} += @bytes;
                    }
                }
            }

            for my $func (keys %{ $rec->{$source}{funcs} }) {
                my ($insns, $bytes) = @{ $rec->{$source}{funcs}{$func} }{qw(instructions bytes)};
                print "$host\t$arch\t$source\t$func\t$insns\t$bytes\n";
            }
        }
    }
}

