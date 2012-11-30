#
#  ntlm-example.pl
#
#
#  Author: Sophie Gu
#
#  $Id: ntlmload-2.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
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


###################################
# ntlmload client start arguments
###################################
our @ntlmload_create_args = ("package", "ntlmload",
			     "config", "-P %%(ts1) -p %%(ts1:tsHttpPort)");

# Case 16: Invalid PDC, also used for case 17, 20
our @ntlmload_start_args1 = ("args", "--pdc_domain TSDEV -c 1 -f url_file.txt -u 1");

#case 21: Bad NTLM requests
our @ntlmload_start_args21 = ("args", "--pdc_domain TSDEV -c 1 -e 1 -g 1.0");

our @empty_args = ();

our $ntlmload_client_instance;

##############################
# Test 16
##############################

# Start up the Traffic Server instance
ntlmload_config("ntlm-16", "whatever.whoknows.com");
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
$ntlmload_client_instance = "ntlmload_client_16";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_case_14");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 150 sec
sleep(150);

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");



##############################
# Test 17
##############################

# Start up the Traffic Server instance
ntlmload_config("ntlm-17", "whatever.whoknows.com, ntlm.inktomi.com");
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
$ntlmload_client_instance = "ntlmload_client_17";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_check_url");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 150 sec
sleep(150);

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");


##############################
# Test 20
##############################

# Start up the Traffic Server instance
ntlmload_config("ntlm-20", "xgu2k-600dp.dhcp.inktomi.com");
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
$ntlmload_client_instance = "ntlmload_client_20";
TestExec::set_log_parser($ntlmload_client_instance, "parse_ntlmload_check_url");
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 150 sec
sleep(150);

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
ntlmload_stop_instance("ts1");


##############################
# Test 21
##############################

# Start up the Traffic Server instance
ntlmload_config("ntlm-21", "ntlm.inktomi.com");
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
$ntlmload_client_instance = "ntlmload_client_21";
TestExec::pm_create_instance($ntlmload_client_instance, "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance($ntlmload_client_instance, \@ntlmload_start_args21);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_client_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# FIX ME: run 300 sec. 
sleep(300);

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ntlmload_stop_instance($ntlmload_client_instance);
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
