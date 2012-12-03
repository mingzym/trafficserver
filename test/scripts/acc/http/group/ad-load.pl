#
#  ad-load.pl
#
#
#  Author: Sophie Gu
#
#  $Id: ad-load.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
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
our $run_time = 300;

#####################################
# Active Directory start arguments
#####################################
our $LDAP_SERVER = "ntlm.inktomi.com";
if ($#ARGV >= 0) {
  $LDAP_SERVER = shift;
}

our $CONFIG = "1";
if ($#ARGV >= 0) {
  $CONFIG = shift;
}

my $CVS_ID_TAG   = '$Id: ad-load.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $';
my $CVS_VER_TAG   = '$Revision: 1.2 $';
my ($CVS_VER)  = $CVS_VER_TAG =~ /([0-9\.]+)/;
my ($CVS_FILE) = $CVS_ID_TAG =~ /Id: (.*),v /;

TestExec::add_to_log("==================================================");
TestExec::add_to_log("$CVS_FILE version $CVS_VER");

#####################################
# Traffic Server start arguments
#####################################
our $cfg = new ConfigHelper;
#$cfg->set_debug_tags ('ldap.*|policy.*');
our $ts_config = $cfg->output;

our $pcfg;
our $pcfg_text;

our @ts_start_args1 = ("args", "-j");
our @ts_start_args2 = ("args", "-Cclear");


###################################
# ldaptest start arguments
###################################
our @ldaptest_create_args = ("package", "ldaptest",
			     "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 300 -i 20000");

our @ldaptest_start_args1 = ("args", "-a 1.0");
our @ldaptest_start_args2 = ("args", "-a 1.0 -e 0.5");
our @ldaptest_start_args3 = ("args", "-g 0.5");
our @ldaptest_start_args4 = ("args", "--cdrop_rate 0.5");
our @ldaptest_start_args5 = ("args", "-a 0.9 -e 0.1 -g 0.1 --cdrop_rate -0.1");
    
our @empty_args = ();

our $ldaptest_client;


##############################
# Test 1 
# -a 1.0
##############################
# Start up the Traffic Server instance
ldaptest_config();
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ldaptest_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ldaptest client instance
$ldaptest_client = "ldaptest_client1";
TestExec::set_log_parser($ldaptest_client, "parse_ntlmload_load");
TestExec::pm_create_instance($ldaptest_client, "%%(load1)", \@ldaptest_create_args);
TestExec::pm_start_instance($ldaptest_client, \@ldaptest_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("LDAPTEST $ldaptest_client started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running
sleep($run_time);

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ldaptest_stop_instance($ldaptest_client);
ldaptest_stop_instance("ts1");


##############################
# Test 2 
# -a 0.5 -e 0.5
##############################
# Start up the Traffic Server instance
ldaptest_config();
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ldaptest_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ldaptest client instance
$ldaptest_client = "ldaptest_client2";
TestExec::set_log_parser($ldaptest_client, "parse_ntlmload_load");
TestExec::pm_create_instance($ldaptest_client, "%%(load1)", \@ldaptest_create_args);
TestExec::pm_start_instance($ldaptest_client, \@ldaptest_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("LDAPTEST $ldaptest_client started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running
sleep($run_time);

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ldaptest_stop_instance($ldaptest_client);
ldaptest_stop_instance("ts1");

##############################
# Test 3
# -g 0.5
##############################
# Start up the Traffic Server instance
ldaptest_config();
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ldaptest_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ldaptest client instance
$ldaptest_client = "ldaptest_client3";
TestExec::set_log_parser($ldaptest_client, "parse_ntlmload_load");
TestExec::pm_create_instance($ldaptest_client, "%%(load1)", \@ldaptest_create_args);
TestExec::pm_start_instance($ldaptest_client, \@ldaptest_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("LDAPTEST $ldaptest_client started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running
sleep($run_time);

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ldaptest_stop_instance($ldaptest_client);
ldaptest_stop_instance("ts1");

##############################
# Test 4
# --cdrop_rate 0.5
##############################
# Start up the Traffic Server instance
ldaptest_config();
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ldaptest_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ldaptest client instance
$ldaptest_client = "ldaptest_client4";
TestExec::set_log_parser($ldaptest_client, "parse_ntlmload_load");
TestExec::pm_create_instance($ldaptest_client, "%%(load1)", \@ldaptest_create_args);
TestExec::pm_start_instance($ldaptest_client, \@ldaptest_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("LDAPTEST $ldaptest_client started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running for 60sec
sleep($run_time);

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ldaptest_stop_instance($ldaptest_client);
ldaptest_stop_instance("ts1");

##############################
# Test 5
# -a 0.9 -e 0.1 -g 0.1 --cdrop_rate 0.1
##############################
# Start up the Traffic Server instance
ldaptest_config();
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
ldaptest_print_config();
TestExec::pm_start_instance("ts1", \@ts_start_args1);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the ldaptest client instance
$ldaptest_client = "ldaptest_client5";
TestExec::set_log_parser($ldaptest_client, "parse_ntlmload_load");
TestExec::pm_create_instance($ldaptest_client, "%%(load1)", \@ldaptest_create_args);
TestExec::pm_start_instance($ldaptest_client, \@ldaptest_start_args1);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("LDAPTEST $ldaptest_client started successfully");
TestExec::add_to_log("--------------------------------------------------");

# Keep test running
sleep($run_time);

# Detect TS crash
$ts_alive = TestExec::is_instance_alive("ts1");
TestExec::add_to_log("ts status $ts_alive");
if ($ts_alive != 1) {
    TestExec::add_to_log("Error: TS crashed");
}

# Stop the load and TS
ldaptest_stop_instance($ldaptest_client);
ldaptest_stop_instance("ts1");


sub ldaptest_stop_instance {
    my ($the_instance) = @_;
  TestExec::pm_stop_instance($the_instance, \@empty_args);
  TestExec::pm_destroy_instance($the_instance, \@empty_args);
  TestExec::add_to_log("--------------------------------------------------");
  TestExec::add_to_log("$the_instance stopped successfully");
  TestExec::add_to_log("--------------------------------------------------");
}

sub ldaptest_config {
  TestExec::add_to_log("------------ldaptest_config ---------------------------");

    $pcfg = new PolicyConfig;
# this works
    $pcfg->LDAP (
		 'name' => "ldap-1",
		 'enabled' => 1,
		 'query-timeout' => '3',
		 'base-dn' => "CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'server-name' => "$LDAP_SERVER",
		 'uid-filter' => "sAMAccountName",
		 'bind-dn' => "CN=uid0,CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'bind-pwd' => "pw0",
		 );
    
# doesn't work (why?)
    $pcfg->LDAP (
		 'name' => "ldap-2",
		 'enabled' => 1,
		 'base-dn' => "DC=tsdev,DC=inktomi,DC=com",
		 'query-timeout' => '3',
		 'server-name' => "$LDAP_SERVER",
		 'uid-filter' => "sAMAccountName",
		 'bind-dn' => "CN=uid0,CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'bind-pwd' => "pw0",
		 );
    
# doesn't work, bad config
    $pcfg->LDAP (
		 'name' => "ldap-3",
		 'enabled' => 1,
		 'query-timeout' => '3',
		 'base-dn' => "DC=tsdev,DC=inktomi,DC=com",
		 'server-name' => "$LDAP_SERVER",
		 'uid-filter' => "distinguishedName",
		 'bind-dn' => "CN=uid0,CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'bind-pwd' => "pw0",
		 );
    
# doesn't work (why?)
    $pcfg->LDAP (
		 'name' => "ldap-4",
		 'enabled' => 1,
		 'query-timeout' => '3',
		 'base-dn' => "DC=tsdev,DC=inktomi,DC=com",
		 'server-name' => "$LDAP_SERVER",
		 'uid-filter' => "CN",
		 'bind-dn' => "CN=uid0,CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'bind-pwd' => "pw0",
		 );
    
# works
    $pcfg->LDAP (
		 'name' => "ldap-5",
		 'enabled' => 1,
		 'query-timeout' => '3',
		 'base-dn' => "CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'server-name' => "$LDAP_SERVER",
		 'uid-filter' => "CN",
		 'bind-dn' => "CN=uid0,CN=Users,DC=tsdev,DC=inktomi,DC=com",
		 'bind-pwd' => "pw0",
		 );
    
    $pcfg->ACL_ALL(auth => "ldap-$CONFIG");
    
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

sub ldaptest_print_config {
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
