#
#  ntlmlod-1.pl
#
#
#  Author: Sophie Gu
#
# $Id: ntlmload-1.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
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

my $CVS_ID_TAG   = '$Id: ntlmload-1.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $';
my $CVS_VER_TAG   = '$Revision: 1.2 $';
my ($CVS_VER)  = $CVS_VER_TAG =~ /([0-9\.]+)/;
my ($CVS_FILE) = $CVS_ID_TAG =~ /Id: (.*),v /;

TestExec::add_to_log("==================================================");
TestExec::add_to_log("$CVS_FILE version $CVS_VER");

#####################################
# Traffic Server start arguments
#####################################
our $cfg = new ConfigHelper;
#$cfg->set_debug_tags ('ntlm.*');
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

# Case 1: Basic authentication: Test the absic functionality (IE)
our @ntlmload_start_args1 = ("args", "--pdc_domain TSDEV -c 1 -f url_file.txt -u 1");

# Case 2: Basic authentication: Test the absic functionality (Netscape)
our @ntlmload_start_args2 = ("args", "--pdc_domain TSDEV -c 1 -f url_file.txt -u 1 -b 1");

# Case 3: Basic authentication: Test the absic functionality (https)
our @ntlmload_start_args3 = ("args", "--pdc_domain TSDEV -c 1 -f url_file_https.txt -u 1");

# Case 14
our @ntlmload_start_args14 = ("args", "--pdc_domain TSDEV -c 1 -f url_file.txt -u 1 -a 0.0");

#case 15 case sensitive nt_domain (INKqa12424)
our @ntlmload_start_args15 = ("args", "--pdc_domain tsdev -c 1 -f url_file.txt -u 1");

our @empty_args = ();

##############################
# Test 1
##############################
# Generate a simple Policy_Config.XML.
$pcfg = new PolicyConfig;
$pcfg->NTLM (
  'name' => "ntlm-def",
  'enabled' => 1,
  'dc-list' => "209.131.55.33",
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

our $pcfg->ACL_ALL(auth => "ntlm-def");
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

TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);

# Header
if (1) {
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

TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
  TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

TestExec::set_log_parser("ntlmload1", "parse_ntlmload_check_url");
TestExec::pm_create_instance("ntlmload1", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload1", \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD instance 1 started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep(10);

# Stop the load and TS
ntlmload_stop_instance("ntlmload1");
ntlmload_stop_instance("ts1");

##############################
# Test 2
##############################
# Don't change ts_config, use the one for Test 1.
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
  TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

TestExec::set_log_parser("ntlmload2", "parse_ntlmload_check_url");
TestExec::pm_create_instance("ntlmload2", "%%(load2)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload2", \@ntlmload_start_args2);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD instance 2 started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep(10);

# Stop the load and TS
ntlmload_stop_instance("ntlmload2");
ntlmload_stop_instance("ts1");


##############################
# Test 3
##############################
# Don't change ts_config, use the one for Test 1.
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

TestExec::set_log_parser("ntlmload3", "parse_ntlmload_check_url");
TestExec::pm_create_instance("ntlmload3", "%%(load3)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload3", \@ntlmload_start_args3);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("NTLMLOAD instance 3 started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep(10);

# Stop the load and TS
ntlmload_stop_instance("ntlmload3");
ntlmload_stop_instance("ts1");


##############################
# Test 14
##############################
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_case_14");
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args14);
TestExec::add_to_log("NTLMLOAD instance 14 started successfully");

# Keep test running for 60sec
sleep(10);

# Stop the load and TS
ntlmload_stop_instance("ntlmload_client");
ntlmload_stop_instance("ts1");


##############################
# Test 15
##############################
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ntlmload client instance
TestExec::set_log_parser("ntlmload_client", "parse_ntlmload_check_url");
TestExec::pm_create_instance("ntlmload_client", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("ntlmload_client", \@ntlmload_start_args15);
TestExec::add_to_log("NTLMLOAD client 15 started successfully");

# Keep test running for 60sec
sleep(10);

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
