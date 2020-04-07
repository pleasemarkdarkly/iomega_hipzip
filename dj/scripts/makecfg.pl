#!/usr/bin/perl

my $BaseDir = $ARGV[0];
my $AppVer = $ARGV[1];
my $CddbVer = $ARGV[2];
my $Count = 0;
my @ContentFiles;
my @CddbFiles;
my $ImageFile;

chdir($BaseDir) or die "can't chdir to $BaseDir\n";

while(defined($file = glob("*.img")))
{
  $ImageFile = $file;
  $Count++;
}

$Count > 1 and die "There can be only one img file in $BaseDir\n";
$Count < 1 and die "No img file found in $BaseDir\n";

(-d "$BaseDir\\cddb" or -d "$BaseDir\\iofreedb") or die "No CDDB directory found\n";
-d "$BaseDir\\cddb" and -d "$BaseDir\\iofreedb" and die "2 CDDB directories found can't continue\n";
opendir(BASEDIR, $BaseDir) or die "Can't open directory $BaseDir: $!\n";

while(defined($file = readdir(BASEDIR)))
{
  if(-d "$BaseDir\\$file")
  {
    if($file !~ /^\.{1,2}$/)
    {
      if($file =~ /^(cddb|iofreedb)$/i)
      {
         GetFiles("$BaseDir\\$file", \@CddbFiles);
      }
      elsif($file =~ /^content$/i)
      {
         GetFiles("$BaseDir\\$file", \@ContentFiles);
      }
      else
      {
        die "unexpected directory $file found in $BaseDir\n";
      }
    }
  }
  else
  {
    if($file !~ /\.img$/i)
    {
      die "unexpected file $file found in directory $BaseDir\n";
    }
  }
}

closedir(BASEDIR);

open(OUTFILE, ">restore.cfg") or die "can't open output file out.cfg\n";
select OUTFILE;

if($#ContentFiles > -1)
{
  print "[content]\n";
  foreach $file(@ContentFiles)
  {
    print "$file\n";
  }
}
if($#CddbFiles > -1)
{
  print "[cddb]\n";
  print "*version=$CddbVer\n";
  foreach $file(@CddbFiles)
  {
    print "$file\n";
  }
}
print "[app]\n";
print "*version=$AppVer\n";
print "/$ImageFile";

sub GetFiles
{
  my $Dir, $FileArrayRef;
  ($Dir, $FileArrayRef) = @_;
  my @SubDirs;
  opendir(DIR, $Dir) or die "can't open directory $Dir: $!\n";
  while(defined($File = readdir(DIR)))
  {
    $FullFile = "$Dir\\$File";
    if(-d $FullFile)
    {
      $File !~ /^\.{1,2}$/ and push(@SubDirs, $FullFile);
    }
    else
    {
      $Chop = $BaseDir;
      $Chop =~ s/\\/\\\\/g;
      $FullFile =~ s/^$Chop//ig;
      $FullFile =~ s/\\/\//g;
      push(@$FileArrayRef, $FullFile);
    }
  }
  closedir(DIR);
  foreach $dir(@SubDirs)
  {
    GetFiles($dir, $FileArrayRef);
  }
}