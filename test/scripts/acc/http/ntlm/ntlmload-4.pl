#
#  ntlm-example.pl
#
#
#  Author: Sophie Gu
#
#  $Id: ntlmload-4.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
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

# Hack
our $previous_port;

our @ntlmload_dc_create_args = ("package", "ntlmload",
				"config", "--no_servers --no_clients ");
our @ntlmload_pdc_start_args1 = ("args", "--pdc_port %%(ntlmload_pdc:dc_port)");
our @ntlmload_bdc_start_args1 = ("args", "--pdc_port %%(ntlmload_bdc:dc_port)");


###################################
# ntlmload client start arguments
###################################
our @ntlmload_create_args = ("package", "ntlmload",
			     "config", "-P %%(ts1) -p %%(ts1:tsHttpPort)");

# Case 18, also used for Case 19.
our @ntlmload_start_args18 = ("args", "--pdc_domain TSDEV -c 300");

our @empty_args = ();

our $ntlmload_client_instance;


##############################
# Test 18
##############################

# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args1);

$previous_port = TestExec::get_var_value('ntlmload_pdc:dc_port');
TestExec::add_to_log("******* previous port is $previous_port **********");

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD server started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Wait PDC to become alive
$r = TestExec::wait_for_server_port("ntlmload_pdc", "%%(ntlmload_pdc:dc_port)", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: PDC failed to startup");
    die "ntlmload_pdc failed to start up\n";
}

# Start up the Traffic Server instance
ntlmload_config("ntlm-18", "%%(load1):%%(ntlmload_pdc:dc_port), ntlm.inktomi.com");
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ntlmload_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ntlmload client instance
TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_load");
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args18);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD client 18 started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep(30);

# Don't use RAF now, it causes caching problem.
#our @raf_pdc_args = "/stats/pdc_sec";
#our @raf_pdc_result = TestExec::raf_instance("ntlmload_pdc", "query", \@raf_pdc_args);

# Must do at five ops per second
#if ($raf_pdc_result[11] < 1) {
#   TestExec::add_to_log("Error: insufficent ntlmload ops: $raf_pdc_result[11]"); 
#} else {
#   TestExec::add_to_log("Status: ntlmload ops: $raf_pdc_result[11]"); 
#}

# Stop synthetic PDC, to make TS use BDC
ntlmload_stop_instance("ntlmload_client");
ntlmload_stop_instance("ntlmload_pdc");

# Make another request, TS should use BDC this time.
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args18);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD client 18 started successfully again");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for more than 1 minutes to make TS retry PDC
sleep(60);

# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
our @ntlmload_pdc_start_args2 = ("args", "--pdc_port $previous_port");
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args2);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD server started successfully again");
TestExec::add_to_log("--------------------------------------------------");

# FIX ME: find out TS uses bdc (probably tcpdump).
#TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_check_url");
TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_load");
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args18);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD client 18 started successfully the third time");
TestExec::add_to_log("--------------------------------------------------");

sleep(60);

#@raf_pdc_result = TestExec::raf_instance("ntlmload_pdc", "query", \@raf_pdc_args);

# Must do at five ops per second
#if ($raf_pdc_result[11] < 1) {
#   TestExec::add_to_log("Error: insufficent ntlmload ops: $raf_pdc_result[11]"); 
#} else {
#   TestExec::add_to_log("Status: ntlmload ops: $raf_pdc_result[11]"); 
#}

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance("ntlmload_client");
ntlmload_stop_instance("ntlmload_pdc");
ntlmload_stop_instance("ts1");

##############################
# Test 19
##############################

# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD pdc started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Wait for the ntlmload port to become live
$r = TestExec::wait_for_server_port("ntlmload_pdc", "%%(ntlmload_pdc:dc_port)", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: ntlmload_pdc failed to startup");
    die "ntlmload_pdc failed to start up\n";
}

# Start the ntlmload bdc instance
TestExec::pm_create_instance("ntlmload_bdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_bdc", \@ntlmload_bdc_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD bdc started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Wait for the ntlmload port to become live
$r = TestExec::wait_for_server_port("ntlmload_bdc", "%%(ntlmload_bdc:dc_port)", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: ntlmload_bdc failed to startup");
    die "ntlmload_bdc failed to start up\n";
}

# Start up the Traffic Server instance
ntlmload_config ("ntlm-19", "%%(load1):%%(ntlmload_pdc:dc_port), %%(load1):%%(ntlmload_bdc:dc_port)");
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ntlmload_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ntlmload client instance
# FIX ME: find out TS uses pdc (probably tcpdump).
#TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_check_url");
TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_load");
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args18);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD client 19 started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep(60);

# Stop synthetic PDC, to make TS use BDC
ntlmload_stop_instance("ntlmload_pdc");

# Wait for 10 sec to make sure PDC and client are stopped.
sleep(10);

# Make another request, TS should use BDC this time.
# FIX ME: find out TS uses bdc (probably tcpdump).

# Keep test running for 60sec
sleep(60);

# Stop BDC
ntlmload_stop_instance("ntlmload_client");
ntlmload_stop_instance("ntlmload_bdc");

# Wait for 10 sec to make sure BDC and client are stopped.
sleep(10);

# FIX ME: find out TS uses bdc (probably tcpdump).
#TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_check_url");
TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_load");
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args18);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD client 19 started successfully the third time");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep(30);

# FIX ME: find out all client requests failed.

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance("ntlmload_client");
ntlmload_stop_instance("ts1");


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
		 'dc-retry-time' => '30',
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
