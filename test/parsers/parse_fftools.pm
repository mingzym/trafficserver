#
#  parse_fftools.pm
#
#   Description:
#     Generic parser for ffqtload, ffwmload2, ffrmload2, and the other ff
#     media load testers.
#
#   $Id: parse_fftools.pm,v 1.2 2003-06-01 18:38:30 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

package parse_fftools;
require Exporter;

use strict Vars;

our @ISA = qw(Exporter);
our @EXPORT = qw();
our @EXPORT_OK = qw();
our $VERSION = 1.00;

sub process_test_log_line {
    my ($instance_id, $level, $line) = @_;

    if ($$line =~ /DESCRIBE returned error 401/ ||
	$$line =~ /Exceeded max number of authentication attempts/ ||
	$$line =~ /80070005/) {
	# in some tests, we would like to see how MIXT logs failed auth 
	# attempts; therefore, we leave it up to the test to check for the
	# appropriate log entries
	return "ok";
    } elsif ($$line =~ /error/i ||
	     $$line =~ /Abort/i ||
	     $$line =~ /Fatal/i ||
	     $level =~ /error/i ) {
	return "error";
    } elsif ($$line =~ /always\]/) {
	return "ok";
    } elsif ($$line =~ /udp warning\] Allocated/) {
	return "ok";
    } elsif ($$line =~ /warning/i ||
	     $level =~ /warning/i) {
	return "warning";
    }  elsif ($level eq "stderr") {
	return "error";
    } else {
	return "ok";
    }
}
