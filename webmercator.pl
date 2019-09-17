#!/usr/bin/perl -w

use strict;
use Math::Trig;
use Data::Dumper;

my $deg2rad = pi / 180.;
my $rad2deg = 180. / pi;

my $a = 6378137;

my ($lon, $lat) = @ARGV;

$lon *= $deg2rad;
$lat *= $deg2rad;

my $X = $a * $lon;
my $Y = $a * log (tan (pi / 4. + $lat * 0.5));

printf (" %d %d\n", $X, $Y);



# Zoom     Résolution (m/px)     Résolution géographique (deg/px) 	Échelle approximative
my @X = 
qw (
 0         156543.0339280410     1.40625000000000000000                    559082264
 1          78271.5169640205     0.70312500000000000000                    279541132
 2          39135.7584820102     0.35156250000000000000                    139770566
 3          19567.8792410051     0.17578125000000000000                     69885283
 4           9783.9396205026     0.08789062500000000000                     34942642
 5           4891.9698102513     0.04394531250000000000                     17471321
 6           2445.9849051256     0.02197265625000000000                      8735660
 7           1222.9924525628     0.01098632812500000000                      4367830
 8            611.4962262814     0.00549316406250000000                      2183915
 9            305.7481131407     0.00274658203125000000                      1091958
10            152.8740565704     0.00137329101562500000                       545979
11             76.4370282852     0.00068664550781250000                       272989
12             38.2185141426     0.00034332275390625000                       136495
13             19.1092570713     0.00017166137695312500                        68247
14              9.5546285356     0.00008583068847656250                        34124
15              4.7773142678     0.00004291534423828120                        17062
16              2.3886571339     0.00002145767211914060                         8531
17              1.1943285670     0.00001072883605957030                         4265
18              0.5971642835     0.00000536441802978516                         2133
19              0.2985821417     0.00000268220901489258                         1066
20              0.1492910709     0.00000134110450744629                          533
21              0.0746455354     0.00000067055225372315                          267
);


my @F;

while (my ($i, $f) = splice (@X, 0, 4))
  {
    push @F, $f;
  }

my $L = 18;


my $X0 = -20037508.3427892476320267;
my $Y0 = +20037508.3427892476320267;

my $F = $F[$L];

$F = 2 * pi * $a / (256*2**$L);

my $Z = 256 * $F;

my $IX = int (($X - $X0) / $Z); # = 132877.090037192
my $IY = int (($Y0 - $Y) / $Z); # =  90241.351534632


print &Dumper ([$IX, $IY]);

my $url = "http://wxs.ign.fr/CLEF/geoportail/wmts?SERVICE=WMTS&VERSION=1.0.0&REQUEST=GetTile&STYLE=normal&LAYER=ORTHOIMAGERY.ORTHOPHOTOS&"
        . "EXCEPTIONS=text/xml&FORMAT=image/jpeg&TILEMATRIXSET=PM&TILEMATRIX=$L&TILEROW=$IY&TILECOL=$IX";


print "$url\n";


__END__

https://geoservices.ign.fr/documentation/geoservices/wmts.html#exemple-de-requ%C3%AAte-wmts-

$ echo "2.478917 48.805639" | proj  +proj=webmerc +datum=WGS84
275951.78       6241946.52


wget -O GetCapabilities.xml 'https://wxs.ign.fr/an7nvfzojv5wa96dsga5nk8w/geoportail/wmts?layer=ORTHOIMAGERY.ORTHOPHOTOS&style=normal&tilematrixset=PM&Service=WMTS&Request=GetCapabilities'

xpath -e '//TileMatrixSet[./ows:SupportedCRS[text()="EPSG:3857"]]/TileMatrix[./ows:Identifier[text()="18"]]' GetCapabilities.xml

TopLeftCorner  -20037508.3427892476320267 20037508.3427892476320267




