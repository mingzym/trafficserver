#!/usr/bin/perl
#
#  run_test.pl
#   Author          : Mike Chowla
#
#   Description:
#
#   $Id: run_test.pl,v 1.2 2003-06-01 18:38:29 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

use strict vars;
use Cwd;
use File::Spec;


our $pkg_suffix = `uname -s`;
chomp($pkg_suffix);

sub check_ts_pkg {

    opendir(PKG_DIR, "packages") || die "Failed to open packages dir : $!\n";

    my @dir_entries = readdir(PKG_DIR);
    close(PKG_DIR);

    my $tmp;
    while ($tmp = shift(@dir_entries)) {
	if ($tmp =~ /^ts-$pkg_suffix/) {
	    return 1;
	} 

    }

    return 0;
}

# Check for help
if ($#ARGV == 0 && $ARGV[0] =~ /^-[hH]/) {
    print "Usage: run_test.pl [<build_dir>] <args to test_exec>\n";
    exit 1;
}

# Check to see if we've been local directory
#  for Traffic Server
my $ts_localpath = "";
if ($ARGV[0] !~ /^-/) {
    $ts_localpath = shift(@ARGV);

    if ($ts_localpath !~ /^\//) {
	# We've got a relative path
	my @path_els = File::Spec->splitdir(getcwd());
	
	$path_els[$#path_els] = $ts_localpath;
	$ts_localpath = File::Spec->catdir(@path_els);

	if ( ! -d $ts_localpath) {
	    die "Can not find TS dir $ts_localpath\n";
	}
    }
} else {
    if (!check_ts_pkg()) {
	die "Can not find TS - localpath not set and no package\n";
    }
}
 
my $test_exec = "packages/test_exec-" . $pkg_suffix;

if (-x $test_exec ) {
    my @exec_args = @ARGV;

    if ($ts_localpath) {
	push(@exec_args, "-w", "ts_localpath=" . "$ts_localpath");
    }

    exec ($test_exec, @exec_args);
} else {
    print "Error: Can not find $test_exec";
    exit 1;
}
