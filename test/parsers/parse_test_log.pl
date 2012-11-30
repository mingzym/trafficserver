#!/inktest/dist/bin/perl
#
#  parse_test_log.pl
#  Author          : Mike Chowla
#
#   Description:
#
#   $Id: parse_test_log.pl,v 1.2 2003-06-01 18:38:30 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

use strict Vars;
use parse_dispatcher;

our @errors = ();
our @warnings = ();
our $line;

our $errors = 0;
our $warnings = 0;

our %args;
while ($tmp_arg = shift(@ARGV)) {
    if (scalar(@ARGV) && $ARGV[0] !~ /^-/) {
	$args{$tmp_arg} = shift(@ARGV);
    } else {
	$args{$tmp_arg} = 1;
    }
}

if (! $args{"-in"}) {
    die "Usage: parse_test.log -in <log_file_name> [-out <out_file)] [-html] \n";
}

our $in_file = $args{"-in"};
open (LOG_IN, "< $in_file") || die "Could not open log file $in_file : $!\n";

our $out_file = $args{"-out"};
if ($out_file) {
    open (LOG_OUT, "> $out_file") || die "Could not open output file $out_file : $!\n";
} else {
    open (LOG_OUT, ">&STDOUT");
}

our $output_html = 0;
our $tmp_body = "";
our $tmp_summary = "";
our $test_name = "Unknown";

if ($args{"-html"}) {
    $output_html = 1;
    $tmp_body = $out_file . ".tmp_body";
    $tmp_summary = $out_file . ".tmp_summary";

    open (TMP_BODY, "> $tmp_body") ||
	die "Could not open tmp file $tmp_body : $!\n";
    open (TMP_SUMMARY, "> $tmp_summary") ||
	die "Could not open tmp file $tmp_summary : $!\n";
}

if ($args{"-testname"}) {
    $test_name = $args{"-testname"};
}


our $problem_count = 0;
while ($line = <LOG_IN>) {

    my $r = parse_dispatcher::process_test_log_line($line);

    if ($r ne "" && $r ne "ok") {
	if ($r eq "error") {
	    $errors++;
	    $problem_count++;
	} elsif ($r eq "warning") {
	    $warnings++;
	    $problem_count++;
	} else {
	    warn("unknown line type from $r\n");
	}

	if ($output_html) {
	    chomp($line);
	    print TMP_SUMMARY "<a href=\"#problem_" . $problem_count . "\">" .
		$r . ": " . $line  . "</a>\n";

	    my $body_str = "<a name=\"problem_" . $problem_count . "\" " .
		"href=\"#problem_" . ($problem_count + 1) . "\">NEXT </a> ";
	    if ($r eq "error") {
		$body_str = $body_str . "<font color=\"red\">";
	    } else {
		$body_str = $body_str . "<font color=\"purple\">";
	    }
	    $body_str = $body_str . $line . "</font>\n";
	    print TMP_BODY $body_str;
	} else {
	    print LOG_OUT "$r: $line";
	}
	
    } else {
	if ($output_html) {
	    chomp($line);
	    print TMP_BODY "      $line\n";
	} 
    }
}


close(LOG_IN);

if ($output_html) {
    close (TMP_BODY);
    close (TMP_SUMMARY);

    open (TMP_BODY, "< $tmp_body") ||
	die "Could not reopen tmp file $tmp_body : $!\n";
    open (TMP_SUMMARY, "< $tmp_summary") ||
	die "Could not reopen tmp file $tmp_summary : $!\n";

    print LOG_OUT "<html>\n<head>\n<title>Test Report for $test_name </title>\n</head>\n";
    print LOG_OUT "<body bgcolor=\"White\">\n<h2> Test Report for $test_name ";
    print LOG_OUT "</h2>\n<h3> Summary: </h3>";
    print LOG_OUT " <h4><font color=\"red\">$errors Errors</font>";
    print LOG_OUT "; <font color=\"purple\">$warnings  Warnings</font></h4>\n<pre>\n";

    my $tmp; 
    while ($tmp = <TMP_SUMMARY>) {
	print LOG_OUT $tmp;
    }
    close (TMP_SUMMARY);
    unlink($tmp_summary);

    print LOG_OUT "</pre>\n<h3> Full Log </h3>\n<pre>\n";

    while ($tmp = <TMP_BODY>) {
	print LOG_OUT $tmp;
    }
    close (TMP_BODY);
    unlink($tmp_body);

    print LOG_OUT "</pre>\n";

    if ($problem_count > 0) {
	print LOG_OUT "<h4><a name=\"#problem_" . ($problem_count + 1) . "\">" .
	    "No More Errors </a></h4>\n";
    }

    print LOG_OUT "</body>\n</html>\n";

} else {
    print LOG_OUT "\n#### $errors Errors; $warnings  Warnings ####\n";
}
close(LOG_OUT);

if ($out_file) {
    print STDOUT "#### $errors Errors; $warnings  Warnings ####w\n";
}

exit(0);







