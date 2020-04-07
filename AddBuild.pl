use strict;
use DBI;
use DBD::ODBC;
 
my $DSN   = 'TrackGearBuilds';
my $UserName  = 'tguser';
my $UserPass = 'brah4';
my $TableName  = 'btsbuild';
my $ProjID  = "Z";
my $Build  = "Z";
my $db;
my $ug;
my $Switch;


#Process the input arguments
while (@ARGV) {

  $Switch = shift(@ARGV);
  if ( $Switch eq "-p") {
     $ProjID = shift(@ARGV);
  } elsif ( $Switch eq "-b") {
     $Build = shift(@ARGV);
  } else {
     print ("Unknown option: ",$Switch,"\n");
  }

}


#make sure that we've got both input arguments.
if ( ($ProjID eq "Z") || ($Build eq "Z")) {

  print "Didn't get both a build and project...\n";

  if ($ProjID eq "Z") {
    print "Need a ProjectID!  Current ID's:\n";
	print "\n";

	$db = DBI->connect("dbi:ODBC:$DSN", $UserName, $UserPass) || die "Could not connect to $DSN\n";
    ##### print the table out
	my $stmnt = $db->prepare("SELECT * FROM BTSHDR") || die "Can't prepare statement";
	$stmnt->execute || die "Can't execute select statement.";
	while((my $Project_ID, my $CurrentBugCount, my $ProjectName) = $stmnt->fetchrow_array){
      print "\t$ProjectName\t$Project_ID\n";
	}
	$stmnt->finish;
	$db->disconnect();

  }  else {
	  print "ProjID: ", $ProjID, "\n";
	  print "but no Build!\n\n";
  }
  die "You lose!  Try again!"; 
}


#####  connect to database
$db = DBI->connect("dbi:ODBC:$DSN", $UserName, $UserPass) || die "Could not connect to $DSN\n";

##### check the legitimacy of the submitted Project ID
my %Project;
my $stmnt = $db->prepare("SELECT * FROM BTSHDR") || die "Can't prepare statement";
$stmnt->execute || die "Can't execute select statement.";
while((my $Project_ID, my $CurrentBugCount, my $ProjectName) = $stmnt->fetchrow_array){
   $Project{$Project_ID} = $ProjectName;
}
$stmnt->finish;

if ( exists($Project{$ProjID}) ) {
  #print "The project ID [",$ProjID,"] is valid.\n\n";   
} else {
  print "The project ID [",$ProjID,"] is not valid.\n\n";
  my $key;
  my $value;
  while (($key, $value) = each(%Project)) {
   print "\t$value\t$key\n";
  }
  die;
}
 

#####  insert $Build into table
my $add = $db->prepare("INSERT INTO $TableName (proj_id, build) VALUES ('$ProjID', '$Build')");
$add->execute || die "Could not execute add line.";
$add->finish;


#####Check to see if there are builds in the current project called "NextBuild"
my $NextBuildString = "NextBuild";
my $nbquery = $db->prepare("SELECT BUILD FROM BTSBUILD WHERE PROJ_ID = ? AND BUILD LIKE '%$NextBuildString%'") || die " Could not prepare NextBuild check statement";
$nbquery->execute($ProjID) || die "Could not execute NextBuild check statement:" . $nbquery->errstr;

my $update = 0;
if ($nbquery->rows == 0) {
  #print "No NextBuilds detected in Project ", $ProjID, "\n";
} else {
  $update = 1;
}

while (my @Temp = $nbquery->fetchrow_array() ) {};  #this is needed for the ODBC driver.
$nbquery->finish();


#####if there is a build then UPDATE "*NextBuild" to the current $Build
if ($update == 1) {
   my $sql = qq{UPDATE BTSDTL SET RESOLVED_BUILD = '$Build' WHERE PROJECT_ID = $ProjID AND RESOLVED_BUILD LIKE '%$NextBuildString%' AND RESOLUTION = 'Fixed'};
   my $ChangeNextBuild = $db->prepare($sql);
   $ChangeNextBuild->execute()|| die "Can't execute UPDATE statement:" . $ChangeNextBuild->errstr;
   $ChangeNextBuild->finish();
}

$db->disconnect();