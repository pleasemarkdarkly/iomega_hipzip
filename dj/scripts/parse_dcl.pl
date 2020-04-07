#!env perl
# Usage:
#      scripts/parse_dcl.pl <target>
#
# Input files:
#  configs/<target>_modules
#  player/<module>/<config>.dcl for each module/config pair in <target>_modules
#
# Output files:
#   builds/<target>/_config.mk           Top level makefile include
#   builds/<target>/_modules.h           Macro definitions for each module
#   builds/<target>/_version.h           Version information for this target
#   builds/<target>/_config_list         List of files generated in config step
#   builds/<target>/<module>/module.mk   Generated from corresponding .dcl
#

#use warnings;
use strict;

use IO::File;
use Data::Dumper;
use File::Compare;
use File::Copy;
use POSIX;

sub parse_dcl($$);          # Takes path and filename, returns "parse tree" hashref
sub get_target_dcls($$);    # Returns a hashref mapping modules to dcls
sub dcl_to_module_mk($$);   # Generates module.mk based on a dcl parse tree
sub dcl_to_headers($$);     # Generates config headers based on dcl parse tree
sub cyg_chomp;              # Cygwin pisses me off.... deal with \r \n nonsense
sub now();                  # Return formatted current date and time
sub copy_if_different($$);  # Copies arg 1 over arg 2 if the differ (except for comments)

select STDERR;

my $target = $ARGV[0];
if (scalar(@ARGV) < 1) {
    print "Usage: $0 <target>\n";
    exit 1;
}

##############################################################################
# Configuration
##############################################################################

my $source_tree="player";
my $build_tree="builds/$target";
my $relative_path="../..";
my @module_dep_list;

##############################################################################
# Main
##############################################################################

print "***** $0: Configuring $build_tree\n";

print "\n*** Module configuration for '$target'\n";

# this is only a one argument function, the second argument is ignored.
# unfortunately emacs doesn't like the prototype having a single '$', and,
# just like a volvo owner is owned by their volvo, i am owned by emacs.
my $modules_hashref = get_target_dcls($target,0);

system "mkdir -p $build_tree";

# dc 10/23/01  permit empty module lists
if (!$modules_hashref) {
    print "Couldn't get module list for target '$target'\n";
    print "Generating stub makefile\n";
    if (! -f "$build_tree/Makefile" ) {
        system "echo '% :: ;' > $build_tree/Makefile";
    }
    exit 0;
}

my %modules = %{$modules_hashref};
my @modules = keys(%modules);

my (@types, @requirements, @headers, @dcl_files);
my (@module_names, @mk_files, @config_list);

print "\n*** Individual module configuration\n";
foreach my $module (keys(%modules)) {
    my $dcl_file = $modules{$module};
# strip version numbers
    (my $module_dest = $module) =~ s|\/v[0-9]+_[0-9]+||g;
    $module_dest =~ s|\/current||g;
    (my $hdr_dest = $module_dest) =~ s|\/v[0-9]+_[0-9]+||g;
    (my $dcl_path = $dcl_file) =~ s|([a-z0-9_\-]+\.[a-z0-9_\-]+)||ig;

    if (! -f $dcl_file) {
        print "Can't process module $module ($dcl_path doesn't exist)\n";
        exit 1;
    }

    my $dcl_data = parse_dcl($dcl_path,$dcl_file);

    if (!defined($dcl_data->{name}[0])) {
        print "Can't process module $module ($dcl_file doesn't specify name)\n";
    }
    if (!defined($dcl_data->{type}[0])) {
        print "Can't process module $module ($dcl_file doesn't specify type)\n";
    }
    push @types, ($dcl_data->{name}[0].'_'.$dcl_data->{type}[0]);
    push @types, ('any_'.$dcl_data->{type}[0]);

    ## inverse the name here for macro clarity (type sorting)
    push @module_names, ($dcl_data->{type}[0].'_'.$dcl_data->{name}[0]);

    push @requirements, @{($dcl_data->{requires})} if defined($dcl_data->{requires});

    push @headers, map {[$module, $hdr_dest, $_]} @{($dcl_data->{export})} if defined($dcl_data->{export});

    push @dcl_files, $dcl_file;

    push @dcl_files, map { "$dcl_path$_" } @{($dcl_data->{include})} if defined($dcl_data->{include}[0]);

    push @mk_files, ($module);


    ## output module.mk
    # include this module dir in the list
    push @config_list, ("$build_tree/$module");

    if (! -d "$build_tree/$module") {
        print "mkdir -p $build_tree/$module\n    ";
        system "mkdir -p $build_tree/$module";
    }

    if( defined($dcl_data->{compile}[0]) ) {
        push @config_list, ("$build_tree/$module/src");
        if (! -d "$build_tree/$module/src") {
            print "mkdir -p $build_tree/$module/src\n    ";
            system "mkdir -p $build_tree/$module/src";
        }
    }
    if( defined($dcl_data->{tests}[0]) ) {
        push @config_list, ("$build_tree/$module/tests");
        if (! -d "$build_tree/$module/tests") {
            print "mkdir -p $build_tree/$module/tests\n    ";
            system "mkdir -p $build_tree/$module/tests";
        }
    }
    dcl_to_module_mk($module, $dcl_data);
    dcl_to_headers($hdr_dest, $dcl_data);
}

#### Do dependency check (before config.mk !!)

print "\n*** Dependency information:\n";
print "Provided: ".join(", ",@types)."\n";
print "Requirements: ".join(", ",@requirements)."\n";

for my $requirement (@requirements) {
    if ((grep {$requirement eq $_ } @types) == 0) {
        print "Unsatisfied requirement: $requirement\n";
        exit 1;
    }
}

print "\nNo unsatisfied requirements\n";

#### Figure out version info

# start developer builds at 1000
my $version_num = 1000;
my $version_name = "Dadio";

if( defined(@ARGV[0]) ) {
    $version_name = "\u@ARGV[0]";
#    $version_name = @ARGV[0];
#    (substr $string, 0, 1) =~ tr|a-z|A-Z|;
}
if( defined(@ARGV[1]) ) {
    $version_num  = @ARGV[1];
} else {
    # try and fetch a build number from an existing _config.mk and increment
    my $old_config_mk = "$build_tree/_config.mk";
    if( -f $old_config_mk ) {
        my $res = `grep "BUILD_VERSION " $old_config_mk`;
        (my $ver = $res) =~ s/[^0-9]*//;
        $ver =~ s/\r\n//;
        $version_num = $ver + 1;
    }
}

#### Do config.mk

my $config_mk_path = "$build_tree/_config.mk";

print "\nWriting $config_mk_path... ";

my $config_mk = new IO::File $config_mk_path, "+>"; #truncate then rw

print $config_mk "# $config_mk_path\n";
print $config_mk "# generated by $0 on ".&now."\n";
print $config_mk "# source file:\n\n";
print $config_mk "MODULE_DEP_LIST := ".join(" ", map{ "$_" } @module_dep_list)."\n";
print $config_mk "CONFIG_LIST_FILE := $relative_path/configs/$target.mk\n\n";
print $config_mk "include \$(CONFIG_LIST_FILE)\n\n";
print $config_mk "TARGET_NAME := $target\n\n";
print $config_mk "BUILD_VERSION := $version_num\n";
print $config_mk "BUILD_VERSION_STR := $version_name\n";
print $config_mk "BASE_DIR := $relative_path\n";
print $config_mk "BUILD_TREE := $build_tree\n";
print $config_mk "SRC_TREE := $relative_path/$source_tree\n";
print $config_mk "ECOS_TREE := $relative_path/ecos/builds/\$(ECOS_BUILD_NAME)/\$(ECOS_BUILD_NAME)_install\n\n";
print $config_mk "CONFIG_FILES := ";
print $config_mk join(" ",map { "$relative_path/$_"} @dcl_files)."\n\n";
print $config_mk "MODULES := ";
print $config_mk join(" ",map { "$_" } @mk_files)."\n";

print $config_mk "\n\n## end $config_mk_path\n";
$config_mk->close();

print "\n";

#### Do _modules.h

my $modules_h_name = "__MODULES_H__";
my $modules_h_path = "$build_tree/_modules.h";
my $modules_h_tmppath = $modules_h_path.".$$";

print "\nWriting $modules_h_path... ";

my $modules_h = new IO::File $modules_h_tmppath, "+>"; #truncate then rw

print $modules_h "//\n";
print $modules_h "// $modules_h_path\n";
print $modules_h "// generated by $0 on ".&now."\n";
print $modules_h "//\n";
print $modules_h "\n";
print $modules_h "#ifndef $modules_h_name\n";
print $modules_h "#define $modules_h_name\n";
print $modules_h "\n";
print $modules_h join("\n", map { (my $var=$_) =~ tr|a-z|A-Z|; "#define DDOMOD_$var" } sort @module_names)."\n"; 
print $modules_h "\n";
print $modules_h "#endif // $modules_h_name\n\n";
print $modules_h "// end $modules_h_path\n";

$modules_h->close();

if (my $res=copy_if_different($modules_h_tmppath,$modules_h_path)) {
    print (($res == 2) ? "Created\n" : "Updated\n");
} else {
    print "Unchanged\n";
}

#### Do _version.h

my $version_h_name = "__VERSION_H__";
my $version_h_path = "$build_tree/_version.h";
my $version_h_tmppath = $version_h_path.".$$";

print "\nWriting $version_h_path... ";

my $version_h = new IO::File $version_h_tmppath, "+>"; #truncate then rw

print $version_h "//\n";
print $version_h "// $version_h_path\n";
print $version_h "// generated by $0 on ".&now."\n";
print $version_h "//\n";
print $version_h "\n";
print $version_h "#ifndef $version_h_name\n";
print $version_h "#define $version_h_name\n";
print $version_h "\n";
print $version_h "#define DDO_VERSION ".$version_num."\n";
print $version_h "#define DDO_VERSION_STR \"".$version_name." v".$version_num."\"\n";
print $version_h "\n";
print $version_h "#endif // $version_h_name\n\n";
print $version_h "// end $version_h_path\n";

$version_h->close();

if (my $res=copy_if_different($version_h_tmppath,$version_h_path)) {
    print (($res == 2) ? "Created\n" : "Updated\n");
} else {
    print "Unchanged\n";
}

#### Do exported headers

print "\nChecking exported header symlinks...\n";

foreach my $header (@headers) {
    (my $module_backup = $header->[1]) =~ s|[^/\r\n]+|..|g;
    my $build_path = "$build_tree/$header->[1]/$header->[2]";
    my $source_path = "./$source_tree/$header->[0]/include/$header->[2]";
    my $relative_source_path = "$module_backup/$relative_path/$source_path";
    if (-f $source_path) {
        push @config_list, ($build_path);
        if (! (-f $build_path) || ((readlink($build_path)) ne $relative_source_path) ) {
            print "linking $build_path to $source_path \n";
            system("ln -sf $relative_source_path $build_path");
        }
    } else {
        print "Could not locate specified header $source_path \n";
    }
}

print "\nCreating Makefile symlink in $build_tree/Makefile\n";
system("ln -sf $relative_path/$source_tree/Makefile $build_tree/Makefile ");

#### Do _config_list

print "\nSyncing configuration...\n";

## Read in last _config_list

my $config_list_path = "$build_tree/_config_list";
my $config_list_file = new IO::File $config_list_path, "r";


if( defined($config_list_file) ) {
    # line by line, determine which items were in the last configuration but
    # not in the current one, and remove those
    my $line;
    while ($line = $config_list_file->getline) {
        $line =~ s/^\s*//;
        $line =~ s/\s*$//;

        my @res = grep(/$line/, @config_list);
        if( scalar(@res) < 1 ) {
            if( -d $line ) {
                system("rm -f $line/*");
                system("rmdir $line");
            } else {
                system("rm -f $line");
            }
        } else {
# this is useful for dumping matches
#        print "$line found in ".join(" ", grep(/$line/, @config_list))."\n";
        }
    }
    $config_list_file->close();
}

# check to see which items are in last_config but not in config_list

my $config_list_tmppath = $config_list_path.".$$";
$config_list_file = new IO::File $config_list_tmppath, "+>"; #truncate then rw

# print the files we generate out. reverse sort them so as to put directories
# after their contents, allowing the above loop to clean properly
print $config_list_file join("\n", sort {$b cmp $a} @config_list)."\n";

$config_list_file->close();

system("mv $config_list_tmppath $config_list_path");



##############################################################################
# Functions
##############################################################################

sub parse_dcl($$) {
    my $dcl_path = $_[0];
    my $dcl_filename = $_[1];
    my ($tests, $include, $dist, $arch, $export, $compile, $link, $basic_link, $link_flags, $build_flags, $type, $name, $requires);
    my $data;

    my @files_to_parse;

    push @files_to_parse, ($dcl_filename);

    foreach my $file (@files_to_parse) {

        my $dcl = new IO::File $file, "r";

        if (!defined($dcl)) {
            print "Couldn't open $file\n";
            return undef;
        }

        my ($line, @line, $x);
        
        $x=1;
        while ($line = $dcl->getline) {
            $line =~ s/#.*$//;
            $line =~ s/^\s*//;
            $line =~ s/\s*$//;
            @line = split /\s+/, $line;

            if ($#line == -1) {
                # ignore blank line
            } elsif ($line[0] =~ /(include)/) {
                push @files_to_parse, map { $dcl_path.$_ } @line[1..$#line];
                push @{$data->{include}}, @line[1..$#line];
            } elsif ($line[0] =~ /(type|name|requires|build_flags|link_flags|tests|dist|arch|export|compile|link|basic_link)/) {
                push @{$data->{$line[0]}}, @line[1..$#line];
            } elsif ($line[0] eq 'header' && $line[2] eq 'start') {
                my $headerline;
                while (($headerline = $dcl->getline) && $headerline !~ /^\s*header\s+$line[1]\s+end\s*$/) {
                    $data->{header}{$line[1]} .= $headerline;
                }
            } else {
                print "Syntax error in $dcl_filename line $x: $line\n";
                exit 1;
            }
            $x++;
        }
        $dcl->close();
    }
    $data->{filename} = $dcl_filename;
    return $data;
}
sub get_target_dcls($$) {
    my ($target) = $_[0];

    my $module_list_filename = "configs/".$target."_modules";
    my $module_list_file = new IO::File $module_list_filename , "r";
    my $module_map;

    if ($module_list_file) {
        my @dcls = $module_list_file->getlines;
        cyg_chomp(@dcls);

        push @module_dep_list, ($relative_path."/configs/".$target."_modules");

      DCL_FILE: foreach my $dcl (@dcls) {
          $dcl =~ s/#.*$//;

          my ($module,$config, @slop) = split /\s+/, $dcl;
          next unless $module;
          # dc 03/26/02 _module list includes
          if ($module =~ /(include)/) {
              # pop open the file, push the contents onto @dcls
              my $include_list_file = new IO::File "configs/".$config;
              if ($include_list_file) {
                  my @new_dcls = $include_list_file->getlines;
                  cyg_chomp(@new_dcls);
                  push @dcls, @new_dcls;
                  $include_list_file->close();
                  push @module_dep_list, ($relative_path."/configs/".$config);
              } else {
                  print "Unable to open file configs/".$config."\n";
              }
              next DCL_FILE;
          }
          if (! -d $source_tree."/".$module) {
              print "Target '$target' references nonexistant module '$module', skipping\n";
              next;
          }
          if (! -f "$source_tree/$module/${config}.dcl") {
              if (! -f "$source_tree/$module/default.dcl") {
                  print "Module '$module' has no configuration for '$config', ";
                  print "and no default was found, skipping.\n";
                  next DCL_FILE;
              } else {
                  $config = 'default';
              }
          }
          $module_map->{$module} = "$source_tree/$module/${config}.dcl";
          print "$module:\n    ${config}.dcl\n";
      }
        return $module_map;
    } else { # build_tree/configs/target_module file not found
        print "Target '$target' is invalid (couldn't open $module_list_filename)\n";
        exit 1;
    }
}

sub cyg_chomp {
    foreach my $item (@_) {
        $item =~ s|[\n\r]+||s;
    }
}

sub dcl_to_module_mk($$) {
    my ($module, $dcl_data) = @_;
    my $module_mk_path = "$build_tree/$module/_module.mk";
    my $module_mk_tmppath = $module_mk_path.".$$";
    my $module_mk = new IO::File $module_mk_tmppath, "+>"; #truncate then rw
    
    push @config_list, ($module_mk_path);

    if (!defined($module_mk)) {
        print "Couldn't open temporary file $module_mk_tmppath for writing\n";
        exit 1;
    }
    print "Checking $module/_module.mk... ";

    print $module_mk "# $module_mk_path\n";
    print $module_mk "# generated by $0 on ".now()."\n";
    print $module_mk "# source file: $dcl_data->{filename}\n\n";

    if (defined($dcl_data->{build_flags}[0])) {
        print $module_mk "COMPILER_FLAGS += ".join(" ",@{$dcl_data->{build_flags}})."\n\n";
    }

    if (defined($dcl_data->{link_flags}[0])) {
        print $module_mk "LINK_FLAGS += ".join(" ",@{$dcl_data->{link_flags}})."\n\n";
    }

    if (defined($dcl_data->{compile}[0])) {
        print $module_mk "SRC += ".join(" ",map
                                        { "$module/src/$_" }
                                        @{$dcl_data->{compile}}
                                        )."\n\n";
    }

    if (defined($dcl_data->{link}[0])) {
        print $module_mk "EXTENDED_LIBS += ".join(" ",map
                                         { "$module/libs/$_" }
                                         @{$dcl_data->{link}}
                                         )."\n\n";
    }
    if (defined($dcl_data->{basic_link}[0])) {
        print $module_mk "LIBS += ".join(" ",map
                                         { "$module/libs/$_" }
                                         @{$dcl_data->{basic_link}}
                                         )."\n\n";
    }
    if (defined($dcl_data->{dist}[0])) {
        print $module_mk "DIST += ".join(" ",map
                                         { "$module/$_" }
                                         @{$dcl_data->{dist}}
                                         )."\n\n";
    }

    if (defined($dcl_data->{tests}[0])) {
        print $module_mk "\n#\n# test cases\n#\n\n";
        print $module_mk "$dcl_data->{name}[0]_$dcl_data->{type}[0]_tests := ";
        print $module_mk join(" ", map
                              { "$module/tests/$_" }
                              @{$dcl_data->{tests}}
                              )."\n\n";
        print $module_mk "$dcl_data->{type}[0]_tests += \$($dcl_data->{name}[0]_$dcl_data->{type}[0]_tests)\n\n";
        print $module_mk "tests += \$($dcl_data->{name}[0]_$dcl_data->{type}[0]_tests)\n\n";
    }

    if (defined($dcl_data->{arch}[0])) {
        (my $dcl_name = $dcl_data->{filename}) =~ s|(\/*.+\/)*||g;
        $dcl_name =~ s|\..+||g;
        print $module_mk "\n#\n# archive lists\n#\n\n";
        print $module_mk "$dcl_data->{name}[0]_$dcl_data->{type}[0]_archive_name := $module/$dcl_name\n\n";
        print $module_mk "$dcl_data->{name}[0]_$dcl_data->{type}[0]_archive := ";
        print $module_mk join(" ", map
                              { "$module/src/$_" }
                              @{$dcl_data->{arch}}
                              )."\n\n";
        print $module_mk "$dcl_data->{type}[0]_archives += $dcl_data->{name}[0]_$dcl_data->{type}[0]_archive\n\n";
        print $module_mk "archives += $dcl_data->{name}[0]_$dcl_data->{type}[0]_archive\n\n";
    }

	print $module_mk "\n## end $module_mk_path\n";

    $module_mk->close();

    if (my $res=copy_if_different($module_mk_tmppath,$module_mk_path)) {
        print (($res == 2) ? "Created\n" : "Updated\n");
    } else {
        print "Unchanged\n";
    }
}

sub dcl_to_headers($$) {
    my ($module, $dcl_data) = @_;

    foreach my $header (keys %{$dcl_data->{header}}) {
        my $header_contents = $dcl_data->{header}{$header};
        (my $include_shield = "$module/$header") =~ tr|-./|___|;
        $include_shield = "__${include_shield}__";

        printf "  Checking $module/$header... ";
        my $header_path = "$build_tree/$module/$header";
        my $header_tmppath = "$header_path.$$";
        my $header_file = new IO::File "$header_tmppath", "+>";

        push @config_list, ($header_path);

        print $header_file "// $header_path\n";
        print $header_file "// generated by $0 on ".now()."\n";
        print $header_file "// source file: $dcl_data->{filename}\n\n";
        print $header_file "#ifndef $include_shield\n#define $include_shield\n\n";
        print $header_file $header_contents;
        print $header_file "\n#endif // $include_shield\n";

        $header_file->close;
        if (my $res=copy_if_different($header_tmppath,$header_path)) {
            print (($res == 2) ? "Created\n" : "Updated\n");
        } else {
            print "Unchanged\n";
        }
    }
}

sub now() {
    return POSIX::strftime "%a %b %d %H:%M:%S %Y", localtime
}

sub copy_if_different($$) {
    my ($tmp, $old) = @_;

    # diff, no errors, no output, ascii mode, ignoring comment lines
    if (! -f $old) {
        if (!move($tmp,$old)) {
            print "mv $tmp $old failed\n";
            exit 1;
        }
        return 2;
    }
#  system "diff -sa -I \"^[//#]\" $old $tmp 2>&1 > /dev/null";
# dc- ignore lines that have 'generated' in them, which is effectively
# the line that has the timestamp
    system "diff -sa -I \"generated\" $old $tmp 2>&1 > /dev/null";
    if ($?) { # return value means files differed
        if (!move($tmp,$old)) {
            print "mv $tmp $old failed\n";
            exit 1;
        }
        return 1;
    } else {
        unlink $tmp;
        return 0
    }
}

