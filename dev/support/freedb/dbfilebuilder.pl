#!/usr/bin/perl

#by chuck ferring, chuckf@fullplaymedia.com

use FileHandle;	#needed for anonymous file handles.

#my $format = "N"; #this defaults to big-endian ordering
my $format = "V"; #uncomment this line to use little-endian ordering.

my $infile = $ARGV[0];
my $outfile = $infile . "\\db.dat";
my $tablefile = $infile . "\\db.tab";
my %files, %current;
my $genre, $currentid;
my $count = 0;
my $currentID;
my $currentOffset = 0;
my $tableOffset = 1020;
my $currentRecord;
my $currentDiscCount = 0;
my $indexstring = pack $format, 1020;
my $tablestring = "";
my $discsinoffset = 0;
my $RecordLength = 1;
my $highcount = 0;
my $highid;
my @currentRecord;
my %Artists;
my $ArtistCount = 0;
my $RawArtistCount = 0;

open(OUTFILE, ">$outfile");
binmode(OUTFILE);
open(TABLEFILE, ">$tablefile");
binmode(TABLEFILE);

select OUTFILE;

opendir(DIR, $infile) or die "couldn't open $infile\n";
while(defined($genre = readdir(DIR)))
{
  if(-d "$infile\\$genre" and $genre !~ /^\.{1,2}$/)
  {
    $genre = $infile . "\\" . $genre;
    $files{$genre} = getfiles($genre);
    $current{$genre} = getfirst($genre);
  }
}

while(!finished())
{
  processdisc(getnext(\%current));
  $count++;
}

print TABLEFILE "$indexstring";
print TABLEFILE "$tablestring";

select STDOUT;

PrintArtists();

print "found $count discs in the database\n";
print "DiscID $highid had $highcount discs\n";
print "found $RawArtistCount total artists in the database\n";
print "found $ArtistCount filtered artists in the database\n";

sub processdisc
{
  my $packed;
  my ($discid, $genre, $currenthash) = @_;
  my $filehandle = $currenthash->{$genre}->[0];
  my @tracks;
  my $count = 0;
  my $Artist;
  my $line = <$filehandle>;
  while($line !~ /^# Track frame offsets/) {$line = <$filehandle>;}
  while(($line = <$filehandle>) =~ /^#\s+\d+/)
  {
    $line =~ s/\D//g;
    push @tracks, $line;
  }
  $packed = pack "c", 255;
  $currentRecord.= $packed;
  push @currentRecord, $packed;
  while($line !~ /^# Disc length:/){$line = <$filehandle>;}
  $line =~ s/\D//g;
  $packed = pack $format, $line;
  $currentRecord.= $packed;
  push @currentRecord, $packed;
  while($line !~ /^DTITLE/){$line = <$filehandle>;}
  chomp $line;
  $line =~ s/^DTITLE=//;
  $line =~ s/\s*$//;
  $Artist = (split " / ", $line)[0];
  AddArtist($Artist);
  $packed = length $line;
  $RecordLength+= (7 + $packed);
  $packed = pack "C", $packed;
  $currentRecord.= $packed;
  push @currentRecord, $packed;
  $currentRecord.= $line;
  push @currentRecord, $line;
  while($line !~ /^DGENRE/){$line = <$filehandle>;}
  chomp $line;
  $line =~ s/^DGENRE=//;
  $line =~ s/^\s*|\s*$//;
  $line eq "" and $line = $genre;
  $line =~ s/^.*\\//;
  $line =~ s/\s*$//;
  $packed = length $line;
  $RecordLength+= $packed;
  $packed = pack "C", $packed;
  push @currentRecord, $packed;
  push @currentRecord, $line;
  while($line !~ /^TTITLE/){$line = <$filehandle>;}
  while($line =~ /^TTITLE/)
  {
    chomp $line;
    $line =~ s/^TTITLE\d+=//;
    $line =~ s/\s*$//;
    $packed = pack "C", 254;
    $currentRecord.= $packed;
    push @currentRecord, $packed;
    $packed = pack $format, $tracks[$count++];
    $currentRecord.= $packed;
    push @currentRecord, $packed;
    $packed = length $line;
    $RecordLength+= (6 + $packed);
    $packed = pack "C", $packed;
    $currentRecord.= $packed;
    push @currentRecord, $packed;
    $currentRecord.= $line;
    push @currentRecord, $line;
    $line = <$filehandle>;
    $tmp = length $currentRecord;
  }
  getnextentry(\@{$currenthash->{$genre}}, $genre);
}
  

sub getnext
{
  my $currenthash = $_[0];
  my @current;
  my $packed;
  my $minkey;
  my $prefix;
  my $minval = "FFFFFFFF";
  foreach $item (keys %$currenthash)
  {
    @current = @{$currenthash->{$item}};
    if($current[1] and (hex($current[1]) < hex($minval)))
    {
      $minval = $current[1];
      $minkey = $item;
    }
  }
  if($minval eq $currentID)
  {
    $currentDiscCount++;
    $currentDiscCount > $highcount and $highcount = $currentDiscCount and $highid = $currentID;
  }
  else
  {
    if($currentDiscCount)
    {
      $prefix = hex(substr $currentID, 0, 2);
      if($prefix != $currentOffset)
      {
        $tableOffset+= (8 * $discsinoffset);
        $indexstring.= pack $format, $tableOffset;
        $currentOffset = $prefix;
        $discsinoffset = 1;
      }
      else
      {
        $discsinoffset++;
      }
      $packed = pack $format, hex($currentID);
      $tablestring.= $packed;
      $packed = pack $format, tell OUTFILE;
      $tablestring.= $packed;
      $tmp = tell OUTFILE;
      $packed = pack "C", $currentDiscCount;
      $currentRecord = $packed . $currentRecord;
      unshift @currentRecord, $packed;
      $packed = pack($format, $RecordLength);
      $currentRecord = $packed . $currentRecord;
      unshift @currentRecord, $packed;
      foreach $item(@currentRecord)
      {
        print $item;
      }
    }
    $RecordLength = 1;
    $currentID = $minval;
    $currentRecord = "";
    $#currentRecord = -1;
    $currentDiscCount = 1;
  }
  return ($minval, $minkey, $currenthash);
}

sub getnextentry
{
  my $arrayref = $_[0];
  my $genre = $_[1];
  my $line;
  my @tmp = @$arrayref;
  my $filehandle = $tmp[0];
  $line = <$filehandle>;
  $arrayref->[1] = 0;
  while($line = <$filehandle>)
  {
    if($line =~ /#FILENAME=/)
    {
      $line =~ s/#FILENAME=//g;
      $line =~ s/\s//g;
      $arrayref->[1] = $line;
      last;
    }
  }
  if(!$arrayref->[1])
  {
    getnextfile($genre, $arrayref);
  }
}
  
sub finished
{
  my @temp;
  foreach $item (keys %current)
  {
    @temp = @{$current{$item}};
    if($temp[1])
    {
      return 0;
    }
  }
  return 1;
}

sub getfiles
{
  my $file = $_[0];
  my @files;
  my $dbfile;
  opendir(FILEDIR, $file) or die "couldn't open $file\n";
  while(defined($dbfile = readdir(FILEDIR)))
  {
    $dbfile !~ /\.{1,2}$/ and unshift @files, "$file\\$dbfile";
  }
  return \@files;
}

sub getnextfile
{
  my $genre = $_[0];
  my $arrayref = $_[1];
  my $dbfile = pop @{$files{$genre}};
  my $line, $discid;
  close($arrayref->[0]);
  !$dbfile and return;
  local $filehandle = FileHandle->new();
  open($filehandle, $dbfile) or die "couldn't open $dbfile\n";
  binmode($filehandle);
  while($line = <$filehandle>)
  {
    if($line =~ /#FILENAME=/)
    {
      $discid = $line;
      $discid =~ s/#FILENAME=//g;
      $discid =~ s/\s//g;
      last;
    }
  }
  $arrayref->[0] = $filehandle;
  $arrayref->[1] = $discid;
}

sub getfirst
{
  my $file = $_[0];
  my $dbfile, $discid;
  my $line;
  local $filehandle = FileHandle->new();
  $dbfile = pop @{$files{$file}};
  open($filehandle, $dbfile) or die "couldn't open $file\n";
  binmode($filehandle);
  while($line = <$filehandle>)
  {
    if($line =~ /#FILENAME=/)
    {
      $discid = $line;
      $discid =~ s/#FILENAME=//g;
      $discid =~ s/\s//g;
      last;
    }
  }
  return [$filehandle, $discid];
}

sub AddArtist
{
  my $Artist = $_[0];
  my @words;
  my @chars;
  my $temp = "";
  $RawArtistCount++;
  $Artist =~ s/^\s*|\s*$//g;
  $Artist =~ s/\s+/ /g;
  foreach $word (split / /, $Artist)
  {
    if($temp ne "")
    {
      $temp.= " ";
    }
    $temp.= ucfirst lc $word;
  }
  $Artist = $temp;
  $Artist =~ /^various/i and $Artist = "Various";
  $Artist =~ /^soundtrack/i and $Artisr = "Soundtrack";
  $Artists{$Artist} == 1 and return;
  if($Artist =~ /^\w+ \w+$/)	#possible name in John Smith format.
  {
    @words = split / /, $Artist;
    $Artists{$word[1] . ", " . $word[0]} = 1 and return;
  }
  @chars = split //, $Artist;
  foreach $char (@chars)
  {
    ord($char) > 255 and return;
  }
  $Artists{$Artist} = 1;
  $ArtistCount++;
}

sub PrintArtists
{
  foreach $key (sort keys %Artists)
  {
    print STDOUT "$key\n";
  }
}
  
