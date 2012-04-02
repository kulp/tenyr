#!/usr/bin/env perl
use strict;
use warnings;

use Pithub;
use LWP::UserAgent;
use HTTP::Request::Common;
use JSON::Any;
use Perl6::Slurp;
use File::Basename qw(dirname fileparse);

use Data::Dumper;

my $here = dirname $0;

my $username = q(kulp);
my $reponame = q(tenyr);
chomp(my $password = slurp "$here/upload.password");

my %args = (
    arch    => shift(@ARGV) || die("provide arch as first argument"),
    version => shift(@ARGV) || die("provide version as second argument"),
);

my $github_url = "https://api.github.com";

my $ua = LWP::UserAgent->new;

my $token = do {
    if (-f "upload.token" and my ($token) = slurp "upload.token") {
        chomp $token;
        $token;
    } else {
        my $req = POST "$github_url/authorizations", Content => qq({"scopes": ["repo"]});
        $req->authorization_basic($username => $password);
        my $auth = $ua->request($req)->content;
        my $parsed = JSON::Any->jsonToObj($auth);

        my $token = $parsed->{token};
        die "Failed to get OAuth token : $auth" unless $token;
        open my $fh, ">", "$here/upload.token";
        print $fh $token;
        $token;
    }
};

my $p = Pithub->new(
    token => $token,
    user => $username,
);

for my $filepath (@ARGV) {
    my %file;
    ($file{stem}, $file{dir}, $file{ext}) = fileparse($filepath, qr/\.\w+$/);
    $file{version} = $args{version};
    $file{arch} = $args{arch};
    $file{name} = "$file{stem}-$file{arch}-$file{version}$file{ext}";
    $file{desc} = "$file{stem} version $file{version} on $file{arch}";
    $file{size} = (stat $filepath)[7];
    $file{type} = "application/octet-stream";

    my $repo = $p->repos->get(repo => $reponame);
    my $d = $p->repos->downloads->new(
        token => $token,
        user => $username,
        repo => $reponame,
    );

    my $result = $d->create(
        data => {
            name         => $file{name},
            size         => $file{size},
            description  => $file{desc},
            content_type => $file{type},
        },
    );

    die "Upload failed" unless $result = $d->upload(
        result => $result,
        file   => $filepath,
    ) and $result->is_success;
}

