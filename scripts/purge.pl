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

my $repo = $p->repos->get(repo => $reponame);
my $d = $p->repos->downloads->list(
    token => $token,
    user => $username,
    repo => $reponame,
);

use Data::Dumper;
for my $e (@{ $d->content }) {
    my $id = $e->{id};
    warn $id;
    $p->repos->downloads->delete(
        token => $token,
        user  => $username,
        repo  => $reponame,
        download_id => $id,
    );
}

