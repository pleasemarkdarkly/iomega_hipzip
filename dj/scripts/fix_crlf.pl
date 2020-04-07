#!/usr/bin/env perl
#
# fix_crlf.pl: convert dos files to unix by adjusting crlf
#

use strict;

use IO::File;
use POSIX;

if (scalar(@ARGV) < 1) {
    print "Usage: $0 <target>\n";
    exit 1;
}

my $filename = @ARGV[0];

if (! -f $filename) {
    print "File $filename not found\n";
    exit 1;
}

my $outfilename = "$filename.tmp";

my $ifile = new IO::File $filename, "r";
my $outfile = new IO::File $outfilename, "w";

binmode $ifile;
binmode $outfile;

my $line;

while ($line = $ifile->getline) {
    $line =~ s/\r\n/\n/;
    print $outfile $line;
}

close $ifile;
close $outfile;

system("cp $filename $filename.save");
system("mv $outfilename $filename");
system("chmod --reference=$filename.save $filename");

