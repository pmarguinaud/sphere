#!/usr/bin/perl -w
#

use strict;
use WWW::Mechanize;
use HTTP::Request::Common;
use FileHandle;

my $ua ='WWW::Mechanize'->new (autocheck => 0);
$ua->agent_alias ('Linux Mozilla');


# https://wxs.ign.fr/ld0jgrlpaway88fw6u4x3h38/geoportail/wmts?layer=ORTHOIMAGERY.ORTHOPHOTOS&style=normal
# &tilematrixset=PM&Service=WMTS&Request=GetTile&Version=1.0.0&Format=image%2Fjpeg
# &TileMatrix=14&TileCol=8180&TileRow=5905

my $lev = 5;

my ($row0, $col0) = (9, 11);
my $d = 4;

my ($rowA, $rowB) = ($row0, $row0 + $d);
my ($colA, $colB) = ($col0, $col0 + $d);

my @imgY;
for my $row ($rowA .. $rowB)
  {
    my @imgX;
    for my $col ($colA .. $colB)
      {
    
        my $url = 'https://wxs.ign.fr/an7nvfzojv5wa96dsga5nk8w/geoportail/wmts?layer=ORTHOIMAGERY.ORTHOPHOTOS&'
                . 'style=normal&tilematrixset=PM&Service=WMTS&Request=GetTile&Version=1.0.0&Format=image%2Fjpeg&'
        	. "TileMatrix=$lev&TileCol=$col&TileRow=$row";
        
        my $file = sprintf ('Tile_%5.5d_%5.5d_%5.5d.jpg', $lev, $col, $row);
        
        unless (-f $file)
	  {
            my $rp = $ua->request (GET $url);
	    if ($rp->is_success ())
	      {
                'FileHandle'->new (">$file")->print ($rp->content ());
	      }
	    else
	      {
                symlink ('missing.jpg', $file);
	      }
	  }
    
	push @imgX, $file;
      }

    my $imgY = sprintf ('Row_%5.5d.jpg', $row);

    my $n = scalar (@imgX);
    system ('montage', -tile => "${n}x1", -geometry => '+0+0', @imgX, $imgY)
      and die ("Cannot montage $imgY");

    unshift @imgY, $imgY;

  }

my $imgA = sprintf ('Full_%5.5d_%5.5d_%5.5d_%5.5d_%5.5d.jpg', $lev, $rowA, $colA, $rowB, $colB);


my $n = scalar (@imgY);
system ('montage', -tile => "1x${n}", -geometry => '+0+0', reverse (@imgY), $imgA)
  and die ("Cannot montage $imgA");

unlink ($_) for (@imgY);




