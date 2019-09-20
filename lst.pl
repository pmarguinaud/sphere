#!/usr/bin/perl -w
#

use strict;
use DB_File;
use Fcntl qw (O_RDWR O_CREAT);

my $db = tie (my %webmercator, 'DB_File', 'webmercator.db', O_RDWR | O_CREAT, 0644, $DB_BTREE);

my @x = sort keys (%webmercator);

print join ("\n", @x, '');


