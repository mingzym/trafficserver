#
#  ntlm-5.pl
#
#
#  Author: Sophie Gu
#
#  $Id: ntlmload-5.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
#

use TestExec;
use ConfigHelper;
use PolicyConfig;
use strict;

our $stdin;
our $stdout;
our $stderr;

our @ts_create_args;
our $ts_local;
our $r;
our $ts_alive;

#####################################
# Traffic Server start arguments
#####################################
our $cfg = new ConfigHelper;
#$cfg->set_debug_tags ('ntlm.*|policy.*');
our $ts_config = $cfg->output;

our $pcfg;
our $pcfg_text;

our @ts_start_args1 = ("args", "-K");
our @ts_start_args2 = ("args", "-Cclear");

#####################################
# domain controlller start arguments
#####################################
our $NTLM_SERVER = "ntlm.inktomi.com";

our @ntlmload_dc_create_args = ("package", "ntlmload",
				"config", "--no_servers --no_clients ");

our @ntlmload_pdc_start_args = 
    (
     "--pdc_port %%(ntlmload_pdc:dc_port)",
     "--pdc_port %%(ntlmload_pdc:dc_port) --pdrop_rate 0.5",
     "--pdc_port %%(ntlmload_pdc:dc_port) -g 0.5",
     "--pdc_port %%(ntlmload_pdc:dc_port) --pdrop_rate 0.1 -g 0.1",
     "--pdc_port %%(ntlmload_pdc:dc_port) --pdrop_rate 0.1 -g 0.1",
     );

our @ntlmload_bdc_start_args = ("args", "--pdc_port %%(ntlmload_pdc:dc_port)");

###################################
# ntlmload client start arguments
###################################
our @ntlmload_create_args = ("package", "ntlmload",
			     "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 300");

# Load 1
our @ntlmload_start_args1 = ("args", "--pdc_domain TSDEV");

# Load 2
our @ntlmload_start_args2 = ("args", "--pdc_domain TSDEV -g 1.0");

# Load 3
our @ntlmload_start_args3 = ("args", "--pdc_domain TSDEV --cdrop_rate 0.5");

# Load 4
our @ntlmload_start_args4 = ("args", "--pdc_domain TSDEV -a 0.5");

# Load 5
our @ntlmload_start_args5 = ("args", "--pdc_domain TSDEV -a 0.9 -b 0.2 -g 0.1 --cdrop_rate 0.1");

our @empty_args = ();

our $ntlmload_client_instance;

our $TEST_CASE;
if ($#ARGV >= 0) {
  $TEST_CASE = shift;
}

our $ntlmload_run_time = 300;

# FIX ME: how to check memory for all these cases?

##############################
# Test 1 
# Normal synthetic client
##############################
# Initialize synthetic PDC if set so.
init_ntlmload_servers();

# Start the ntlmload client instance
$ntlmload_client_instance = "ntlmload_1";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_load");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep($ntlmload_run_time);

# FIX ME: check ntlmload output using RAF.
# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");
if($TEST_CASE =~ /([0-4])/) {
    ntlmload_stop_instance("ntlmload_pdc");
    if($TEST_CASE eq "4") {
	ntlmload_stop_instance("ntlmload_bdc");
    }
}

##############################
# Test 2
# -g 1.0
##############################
# Initialize synthetic PDC if set so.
init_ntlmload_servers();

# Start the ntlmload client instance
$ntlmload_client_instance = "ntlmload_2";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_load");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args2);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep($ntlmload_run_time);

# FIX ME: check ntlmload output using RAF.
# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");
if($TEST_CASE =~ /([0-4])/) {
    ntlmload_stop_instance("ntlmload_pdc");
    if($TEST_CASE eq "4") {
	ntlmload_stop_instance("ntlmload_bdc");
    }
}

##############################
# Test 3
# --cdrop_rate 0.5
##############################
# Initialize synthetic PDC if set so.
init_ntlmload_servers();

# Start the ntlmload client instance
$ntlmload_client_instance = "ntlmload_3";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_load");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args3);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep($ntlmload_run_time);

# FIX ME: check ntlmload output using RAF.
# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");
if($TEST_CASE =~ /([0-4])/) {
    ntlmload_stop_instance("ntlmload_pdc");
    if($TEST_CASE eq "4") {
	ntlmload_stop_instance("ntlmload_bdc");
    }
}

##############################
# Test 4
# -a 0.5
##############################
# Initialize synthetic PDC if set so.
init_ntlmload_servers();

# Start the ntlmload client instance
$ntlmload_client_instance = "ntlmload_4";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_load");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args4);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep($ntlmload_run_time);

# FIX ME: check ntlmload output using RAF.
# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");
if($TEST_CASE =~ /([0-4])/) {
    ntlmload_stop_instance("ntlmload_pdc");
    if($TEST_CASE eq "4") {
	ntlmload_stop_instance("ntlmload_bdc");
    }
}

##############################
# Test 5
# -a 0.9 -b 0.2 -g 0.1 --cdrop_rate 0.1
##############################
# Initialize synthetic PDC if set so.
init_ntlmload_servers();

# Start the ntlmload client instance
$ntlmload_client_instance = "ntlmload_5";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_load");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args5);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep($ntlmload_run_time);

# FIX ME: check ntlmload output using RAF.
# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");
if($TEST_CASE =~ /([0-4])/) {
    ntlmload_stop_instance("ntlmload_pdc");
    if($TEST_CASE eq "4") {
	ntlmload_stop_instance("ntlmload_bdc");
    }
}



sub ntlmload_stop_instance {
    my ($the_instance) = @_;
  TestExec::pm_stop_instance($the_instance, \@empty_args);
  TestExec::pm_destroy_instance($the_instance, \@empty_args);
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("$the_instance stopped successfully");
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("             Stop Tests");
}

sub ntlmload_config {
    my $test_case = shift;
  TestExec::add_to_log("------------ntlmload_config $test_case-----------------");

    $pcfg = new PolicyConfig;
    $pcfg->NTLM (
		 'name' => $test_case,
		 'enabled' => 1,
		 'dc-list' => shift,
		 'dc-load-balance' => '1',
		 'dc-max-connections' => "3",
		 'dc-max-conn-time' => "1800",
		 'nt-domain' => 'TSDEV',
		 'nt-host' => 'traffic_server',
		 'queue-len' => '10',
		 'req-timeout' => '20',	    
		 'dc-retry-time' => '300',
		 'dc-fail-threshold' => '5',
		 'fail-open' => '0',
		 'allow-guest-login' => '0',
		 );   

    our $pcfg->ACL_ALL(auth => $test_case);
    $pcfg_text = $pcfg->config;
    $ts_config .= $pcfg_text;
    
    @ts_create_args = ("package", "ts",
		       "config", $ts_config);

# Check to see if we are using localpath ts
    $ts_local = TestExec::get_var_value("ts_localpath");
    if ($ts_local) {
	print "Using ts_localpath: $ts_local\n";
	push(@ts_create_args, "localpath", $ts_local);
    }
}

sub ntlmload_print_config {
    my $proxy_port = TestExec::get_var_value('ts1:tsHttpPort');
    my $proxy_server = TestExec::get_var_value('ts1');
    
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("Traffic Server HTTP port: $proxy_port");
  TestExec::add_to_log("Traffic Server HTTP host: $proxy_server");
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("Policy Config:");
    foreach my $ln ((split /\n/, $pcfg_text)) {
      TestExec::add_to_log("  $ln");
    }
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("             Starting Tests");
}

sub init_ntlmload_servers {
  TestExec::add_to_log("TEST_CASE is $TEST_CASE");

    if($TEST_CASE =~ /([0-4])/) {
# Start the ntlmload pdc instance
	my @my_start_args = ("args", $ntlmload_pdc_start_args[$TEST_CASE]);
      TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
      TestExec::pm_start_instance("ntlmload_pdc", \@my_start_args);
	
# Wait PDC to become alive
	$r = TestExec::wait_for_server_port("ntlmload_pdc", "%%(ntlmload_pdc:dc_port)", 60000);
	if ($r < 0) {
	  TestExec::add_to_log("Error: PDC failed to startup");
	    die "ntlmload_pdc failed to start up\n";
	}
    }
    
    if($TEST_CASE eq "4") {
# Start the ntlmload bdc instance
      TestExec::pm_create_instance("ntlmload_bdc", "%%(load2)", \@ntlmload_dc_create_args);
      TestExec::pm_start_instance("ntlmload_bdc", \@ntlmload_bdc_start_args);
	
# Wait BDC to become alive
	$r = TestExec::wait_for_server_port("ntlmload_bdc", "%%(ntlmload_bdc:dc_port)", 60000);
	if ($r < 0) {
	  TestExec::add_to_log("Error: PDC failed to startup");
	    die "ntlmload_bdc failed to start up\n";
	}	
    }
    
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("NTLMLOAD syn server(s)  started successfully");
  TestExec::add_to_log("--------------------------------------------------");  
    
# Start up the Traffic Server instance
# FIX ME: ugle!!! User better form of regex.
    if ($TEST_CASE eq "5") {
	ntlmload_config("ntlm-def", "$NTLM_SERVER");
    } elsif ($TEST_CASE eq "4") {
	ntlmload_config("ntlm-def", 
			"%%(load1):%%(ntlmload_pdc:dc_port), %%(load2):%%(ntlmload_bdc:dc_port)");
    } else {
	ntlmload_config("ntlm-def", "%%(load1):%%(ntlmload_pdc:dc_port)");
    }
    
  TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
    ntlmload_print_config();
  TestExec::pm_start_instance("ts1", \@ts_start_args1);
    
# Wait for the http port to become live on TS
    $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
      TestExec::add_to_log("Error: TS failed to startup");
	die "TS failed to start up\n";
    }
}

# Don't use RAF now, it causes caching problem.
#our @raf_pdc_args = "/stats/pdc_sec";
#our @raf_pdc_result = TestExec::raf_instance("ntlmload_pdc", "query", \@raf_pdc_args);

# Must do at five ops per second
#if ($raf_pdc_result[11] < 1) {
#   TestExec::add_to_log("Error: insufficent ntlmload ops: $raf_pdc_result[11]"); 
#} else {
#   TestExec::add_to_log("Status: ntlmload ops: $raf_pdc_result[11]"); 
#}
