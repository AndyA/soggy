#!/usr/bin/env perl

use feature ":5.10";

use strict;
use warnings;

use Path::Class;
use URI::data;

while ( my $file = shift ) {
  my $data = file( $file )->slurp;
  my $uri  = URI->new( 'data:' );
  $uri->media_type( 'application/vnd.apple.mpegurl' );
  $uri->data( $data );
  say $uri->as_string;
}

# vim:ts=2:sw=2:sts=2:et:ft=perl

