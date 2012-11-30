#
#  parse_sdktest.pm
#  Author          : franckc 
#
#   Description: Parser for SDKTest Server and Client output
#
#   $Id: 
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

package parse_sdktest;
require Exporter;

use strict Vars;

our @ISA = qw(Exporter);
our @EXPORT = qw();
our @EXPORT_OK = qw();
our $VERSION = 1.00;

# Unfortunately, SDKtest uses stderr to output some
# non-error related messages.

sub process_test_log_line {
    my ($instance_id, $level, $line) = @_;

    if ($$line =~ /error/i ||
	$$line =~ /Abort/i ||
	$$line =~ /Fatal/i ||
	$level =~ /error/i ) {
	return "error"
     } elsif ($$line =~ /warning/i ||
	      $level =~ /warning/i) {
	 return "warning";
     } else {
	 return "ok";
     }
}
