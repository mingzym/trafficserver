package MixtUtils;

require Exporter;
require 5.6.0;

use strict vars;

our @ISA = qw(Exporter);
our @EXPORT = qw(get_qt_bandwidth get_wmt_od_bandwidth parse_log_entry);
push @EXPORT, qw(mixt_records_config mixt_storage_config mixt_logs_xml_config);
push @EXPORT, qw(testcase fatal_error wait_for_port raf_cmd check_log_values check_matching_logs);
push @EXPORT, qw(stop_destroy_instance);

###########################################################################
# .config file excerpts that will go into most test scripts' TS config blob
###########################################################################

our $mixt_records_config = <<EOC;

proxy.config.raf.enabled               1

proxy.config.log2.custom_logs_enabled  1
proxy.config.log2.xml_logs_config      1

proxy.config.qt.enabled                1
proxy.config.wmt.enabled               1
proxy.config.rni.enabled               1
proxy.config.cache.storage_filename    storage.config
proxy.config.proxy_name                deft-proxy.inktomi.com
proxy.config.hostdb.size               50000

add CONFIG proxy.config.diags.debug.enabled        INT 1
add CONFIG proxy.config.diags.output.diag          STRING L
add CONFIG proxy.config.diags.output.debug         STRING L
add CONFIG proxy.config.diags.output.status        STRING L
add CONFIG proxy.config.diags.output.note          STRING L
add CONFIG proxy.config.diags.output.warning       STRING L
add CONFIG proxy.config.diags.output.error         STRING EL
add CONFIG proxy.config.diags.output.fatal         STRING EL
add CONFIG proxy.config.diags.output.alert         STRING EL
add CONFIG proxy.config.diags.output.emergency     STRING EL

EOC

our $mixt_storage_config = <<EOC;
. 140000000
EOC

our $mixt_logs_xml_config = <<EOC;
<!-- Media-IXT DEFT log format -->
<LogFormat>
    <Name="deft-mixt"/>
    <Format="%<cqtn> || %<cqts> || %<tfcb> || %<ttms> || %<chi> || %<cqtx> || %<cqu> || %<cquc> || %<cgid> || %<caun> || %<band> || %<fsiz> || %<shn> || %<prcb> || %<prob> || %<pqsi> || %<pqsn> || %<phr> || %<pscl> || %<psql> || %<styp>"/>
</LogFormat>

<!-- Media-IXT DEFT log object -->
<LogObject>
    <Format="deft-mixt"/>
    <Filename="deft-mixt"/>
    <Protocols="mixt"/>
</LogObject>
EOC

###########################################################################
# general helper subs
###########################################################################

sub testcase {
    my ($descr, $tcsub) = @_;
    print("$descr\n");
    TestExec::add_to_log("Status $descr");

    &$tcsub();
}

sub fatal_error {
    my ($msg) = @_;

    TestExec::add_to_log("Error: $msg");
    die($msg);
}

# waits for a port and throws and exception if there was a problem
sub wait_for_port {
    my ($iname, $port, $timeout) = @_;
    my $r;

    $port = TestExec::get_var_value($port);
    $r = TestExec::wait_for_server_port($iname, $port, $timeout);
    if ($r < 0) {
	fatal_error("Failed while waiting for port $port (instance $iname)");
    }
}

# issues RAF command; throws an exception if command fails
sub raf_cmd {
    my ($iname, $cmd, @args) = @_;
    my @raf_result;

    @raf_result = TestExec::raf_instance($iname, $cmd, \@args);
    if ($raf_result[0] < 0) {
	fatal_error("Command $cmd " . join(' ', @args) . "failed; response: " . join(' ', @raf_result));
    }

    return(@raf_result);
}

sub stop_destroy_instance {
   my ($iname, @args) = @_;
 TestExec::pm_stop_instance($iname, \@args);
 TestExec::pm_destroy_instance($iname, \@args);

}

###########################################################################
# format-specific helper subs
###########################################################################

# Return the bandwidth that an instance is getting from the proxy
sub get_qt_bandwidth {
    my ($tool_inst_name, $client_inst_name) = @_;
    my @raf_args = "/ffqtload/instances/" . $client_inst_name . "/*/rtp/total/bps";
    my @raf_result = TestExec::raf_instance($tool_inst_name, "query", \@raf_args); 
    
    if ($raf_result[1] < 0) {
      TestExec::add_to_log("Error: b/w checking failed: $raf_result[1]");
	return 0;
    }
  TestExec::add_to_log("Client b/w: $raf_result[2]");
    return $raf_result[2];
}

sub get_wmt_od_bandwidth {
    my ($tool_inst_name, $client_inst_name) = @_;
    my @raf_args = "/ffwmload/instances/" . $client_inst_name . "/*/ctrl/mms/*/udp/in/bps";
    my @raf_result = TestExec::raf_instance($tool_inst_name, "query", \@raf_args); 
    
    if ($raf_result[1] < 0) {
      TestExec::add_to_log("Error: b/w checking failed: $raf_result[1]");
	return 0;
    }
  TestExec::add_to_log("Client b/w: $raf_result[2]");
    return $raf_result[2];
}


# to be used for the responses of TSLogReader's match_log method
sub parse_log_entry {
    my $log_entry = shift;
    my @field_list = @_;
    my @fields;
    my %field_hash;
    my $i;

    @fields = split(/ \|\| /, $log_entry);
    if (scalar(@field_list) != scalar(@fields)) {
	die sprintf("Error: number of fields (%d) does not match expected (%d)!\n",
		    scalar(@fields), scalar(@field_list));
    }

    for ($i = 0; $i < scalar(@fields); $i++) {
	$field_hash{$field_list[$i]} = $fields[$i];
    }

    return %field_hash;
}

# count_matching_logs(tsmon instance name, constraint1, constraint2, ...)
#
# returns number of logs that match the given constraints
sub count_matching_logs {
    my ($iname, @raf_args) = @_;
    my @raf_result;

    unshift(@raf_args, "match_log");

    @raf_result = raf_cmd($iname, @raf_args);
    if ($raf_result[0] != 0) {
	fatal_error("match_log error: " . join(' ', @raf_result));
    }
    
    return(scalar(@raf_result) - 1);
}


sub check_matching_logs {
    my ($iname, $count_expr, $errmsg, @constraints) = @_;
    my ($count, $r, $op, $rvalue);

    ($op, $rvalue) = split(/s+/, $count_expr);
    $count = count_matching_logs($iname, @constraints);
    eval "\$r = ($count $op $rvalue)";
    if (!$r) {
	$errmsg =~ s/\[val\]/$count/g;
	$errmsg =~ s/\[exp\]/$count_expr/g;
	TestExec::add_to_log("Error: $errmsg");
	print "Error: $errmsg\n";
    }
}
