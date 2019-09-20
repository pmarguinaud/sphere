#!/usr/bin/perl -w
#

use strict;
use DB_File;
use Fcntl qw (O_RDWR O_CREAT);
use FileHandle;

my $db = tie (my %webmercator, 'DB_File', 'webmercator.db', O_RDWR | O_CREAT, 0644, $DB_BTREE);

for my $x (@ARGV)
  {
    if (exists $webmercator{$x})
      {
        'FileHandle'->new (">$x")->print ($webmercator{$x});
      }
    else
      {
        warn ("`$x' was not found");
      }
  }


