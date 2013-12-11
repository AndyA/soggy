#!/usr/bin/env perl

use strict;
use warnings;
use autodie;

use JSON;

my $cuts = JSON->new->decode(
  do { local $/; <> }
);

open my $in, '<', $cuts->{filename};
$in->binmode;

my $idx  = -1;
my $prev = 0;
for my $cl ( @{ $cuts->{clusters} } ) {
  dd( $in, segname( $cuts->{filename}, $idx++ ), $cl->{offset} - $prev );
  $prev = $cl->{offset};
}
dd( $in, segname( $cuts->{filename}, $idx++ ) );

sub dd {
  my ( $in, $outname, $len ) = @_;
  open my $out, '>', $outname;
  $out->binmode;

  while () {
    my $chunk = 1024 * 1024;
    if ( defined $len ) {
      last if 0 == $len;
      $chunk = $len if $chunk > $len;
    }
    my $got = sysread $in, my ($buf), $chunk;
    last if $got == 0;
    syswrite $out, $buf, $got;
    $len -= $got if defined $len;
  }
}

sub segname {
  my ( $base, $idx ) = @_;
  my $name = $idx >= 0 ? sprintf( '%08d', $idx ) : 'hdr';
  $base =~ s/(\.[^.]+)$/-$name$1/g;
  return $base;
}

# vim:ts=2:sw=2:sts=2:et:ft=perl

