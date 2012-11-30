#
#  parse_loadgen.pm
#  Author          : Mike Chowla
#
#   Description:
#
#   $Id: parse_loadgen.pm,v 1.2 2003-06-01 18:38:30 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

package parse_loadgen;
require Exporter;

use strict Vars;

our @ISA = qw(Exporter);
our @EXPORT = qw();
our @EXPORT_OK = qw();
our $VERSION = 1.00;

sub process_test_log_line {
    my ($instance_id, $level, $line) = @_;

    if ($level eq "stderr") { 
         return "error";
     } else {
	 return "ok";
     }
}
