#
#  ntlmload-3.pl
#
#
#  Author: Sophie Gu
#
#  $Id: ntlmload-3.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
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
$cfg->set_debug_tags ('ntlm.*|policy.*');
our $ts_config = $cfg->output;

our $pcfg;
our $pcfg_text;

our @ts_start_args1 = ("args", "-K");
our @ts_start_args2 = ("args", "-Cclear");

#####################################
# domain controlller start arguments
#####################################

our @ntlmload_dc_create_args = ("package", "ntlmload",
				"config", "--no_servers --no_clients ");
our @ntlmload_pdc_start_args = ("args", "--pdc_port %%(ntlmload_pdc:dc_port)");

###################################
# ntlmload client start arguments
###################################
our $ntlmload_instance;
our @ntlmload_create_args = ("package", "ntlmload",
			     "config", "-P %%(ts1) -p %%(ts1:tsHttpPort)");

# Case 10, also used for Case 11,12 and 13
our @ntlmload_start_args1 = ("args", "-c 1 -f url_file.txt -u 1");

our @empty_args = ();


##############################
# Test 10 (bypass host)
##############################
# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args);

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
ntlmload_rule_config("host", "host");
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
$ntlmload_instance = "ntlmload_client_10";
TestExec::set_log_parser("$ntlmload_instance", "parse_ntlmload_case_14");
TestExec::pm_create_instance("$ntlmload_instance", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("$ntlmload_instance", \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 150 sec
sleep(30);

# Stop the load and TS
ntlmload_stop_instance("$ntlmload_instance");
ntlmload_stop_instance("ts1");
ntlmload_stop_instance("ntlmload_pdc");


##############################
# Test 11 (bypass domain)
##############################
# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args);

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
ntlmload_rule_config("dest_domain", "domain");
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
$ntlmload_instance = "ntlmload_client_11";
TestExec::set_log_parser("$ntlmload_instance", "parse_ntlmload_case_14");
TestExec::pm_create_instance("$ntlmload_instance", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("$ntlmload_instance", \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 30 sec
sleep(30);

# Stop the load and TS
ntlmload_stop_instance("$ntlmload_instance");
ntlmload_stop_instance("ts1");
ntlmload_stop_instance("ntlmload_pdc");


##############################
# Test 12 (bypass IP)
##############################
# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args);

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
ntlmload_rule_config("dest_ip", "ip");
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
$ntlmload_instance = "ntlmload_client_12";
TestExec::set_log_parser("$ntlmload_instance", "parse_ntlmload_case_14");
TestExec::pm_create_instance("$ntlmload_instance", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("$ntlmload_instance", \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 30 sec
sleep(30);

# Stop the load and TS
ntlmload_stop_instance("$ntlmload_instance");
ntlmload_stop_instance("ts1");
ntlmload_stop_instance("ntlmload_pdc");


##############################
# Test 13 (bypass regex)
##############################
# Start the ntlmload pdc instance
TestExec::pm_create_instance("ntlmload_pdc", "%%(load1)", \@ntlmload_dc_create_args);
TestExec::pm_start_instance("ntlmload_pdc", \@ntlmload_pdc_start_args);

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
ntlmload_rule_config("key", "regex");
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
$ntlmload_instance = "ntlmload_client_13";
TestExec::set_log_parser("$ntlmload_instance", "parse_ntlmload_case_14");
TestExec::pm_create_instance("$ntlmload_instance", "%%(load1)", \@ntlmload_create_args);
TestExec::pm_start_instance("$ntlmload_instance", \@ntlmload_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("$ntlmload_instance started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 30 sec
sleep(30);

# Stop the load and TS
ntlmload_stop_instance("$ntlmload_instance");
ntlmload_stop_instance("ts1");
ntlmload_stop_instance("ntlmload_pdc");


sub ntlmload_stop_instance {
    my ($the_instance) = @_;
  TestExec::pm_stop_instance($the_instance, \@empty_args);
  TestExec::pm_destroy_instance($the_instance, \@empty_args);
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("$the_instance stopped successfully");
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("             Stop Tests");
}

sub ntlmload_rule_config {
    my ($key_type, $key_method) = @_;
    my %host_list = (
	"host"   => ["www.yahoo.com", "www.inktomi.com", "www.google.com"],
	"domain" => ["yahoo.com", "inktomi.com", "google.com"],
	"ip"     => ["66.218.71.83", "209.131.63.206", "216.239.39.101"],
	"regex"  => [".*yahoo.com", ".*inktomi.com", ".*google.com"],
    );

  TestExec::add_to_log("------------ntlmload_config $key_type-----------------");

# Generate a simple NTLM config in Policy_Config.XML.
    $pcfg = new PolicyConfig;
    $pcfg->NTLM (
		 'name' => "ntlm-def",
		 'enabled' => 1,
		 'dc-list' => "%%(load1):%%(ntlmload_pdc:dc_port)",
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

#  TestExec::add_to_log("$key_type, $key_method, $host_list{$key_type}");
    $pcfg->KEY (
		"key1",
		$pcfg->CRITERIA (type   => $key_type,
				 method => $key_method,
				 value  => $host_list{$key_method}[0]),
		);
    
    $pcfg->KEY (
		"key2",
		$pcfg->CRITERIA (type   => $key_type,
				 method => $key_method,
				 value  => $host_list{$key_method}[1]),
		);
    
    $pcfg->KEY (
		"key3",
		$pcfg->CRITERIA (type   => $key_type,
				 method => $key_method,
				 value  => $host_list{$key_method}[2]),
		);

    $pcfg->ACL (
		"TE:http",
		$pcfg->RULE ( 
			     keyId => "key1", 
			     auzn  => "allow-cfg",
			     ),
		$pcfg->RULE ( 
			     keyId => "key2", 
			     auzn  => "allow-cfg",
			     ),
		
		$pcfg->RULE ( 
			     keyId => "key3", 
			     auth  => "ntlm-def",
			     ),
		);
    
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
