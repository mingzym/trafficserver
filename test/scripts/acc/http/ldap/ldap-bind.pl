#
#  ldap-1.pl (based on append_function by frackc)
#
#    Run functional test cases for plugin append-transform
#
#  Author: bevans
#
#  $Id: ldap-bind.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
#

package main;
use TestExec;
use ConfigHelper;
use PolicyConfig;
use strict;

our $stdin;
our $stdout;
our $stderr;

our $LDAP_SERVER = "bleep.inktomi.com";
if ($#ARGV >= 0) {
  $LDAP_SERVER = shift;
}

my $CVS_ID_TAG   = '$Id: ldap-bind.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $';
my $CVS_VER_TAG   = '$Revision: 1.2 $';
my ($CVS_VER)  = $CVS_VER_TAG =~ /([0-9\.]+)/;
my ($CVS_FILE) = $CVS_ID_TAG =~ /Id: (.*),v /;

# get_var_value(String var)
# set_var_value(String var, String value); 
# Return Value: 0 on success, 1 on failure

TestExec::add_to_log("==================================================");
TestExec::add_to_log("$CVS_FILE version $CVS_VER");

#update tarball
if (1) {
    chomp(my $ldap_path = `dirname $0`);
    chomp(my $deft_path = `pwd`);
    # my $deft_path = $ENV{PWD};

    # TestExec::add_to_log("Note: ldap_path = $ldap_path");
    # TestExec::add_to_log("Note: deft_path = $deft_path");
    die "chdir failed" unless chdir $ldap_path;
    unlink 'ldap_syntest.tar';
    system q(tar cf ldap_syntest.tar Tests);
    die "chdir failed" unless chdir $deft_path;
}

# TS configuration
our $cfg = new ConfigHelper;
$cfg->set_debug_tags ('http_auth|http_hdrs|ldap.*|policy.*');
our $ts_config = $cfg->output;

# Generate a simple Policy_Config.XML.
our $pcfg = new PolicyConfig;
$pcfg->LDAP (
  'name' => "ldap-1",
  'enabled' => 1,
  'base-dn' => "dc=qa,dc=wumpus,dc=org",
  'query-timeout' => '3',
  'server-name' => "$LDAP_SERVER",
  'uid-filter' => "uid",
  'attribute-name' => 'userGroup',
  'attribute-value' => '0',
  'bind-dn' => 'uid=uid0, dc=qa, dc=wumpus, dc=org',
  'bind-pwd' => 'pw0',
#  'secure-bind' => '1',
#  'attribute-value' => '0',
);

our $pcfg->ACL_ALL(auth => "ldap-1");
our $pcfg_text = $pcfg->config;
$ts_config .= $pcfg_text;

our @ts_create_args = 
    ("package", "ts", "config", $ts_config);

# Check to see if we are using localpath ts
my  $ts_local = TestExec::get_var_value("ts_localpath");
if ($ts_local) {
    print "Using ts_localpath: $ts_local\n";
    push(@ts_create_args, "localpath", $ts_local);
}

our @ts_start_args = ("args", "-j");

our @syntest_create_args = 
    ( "package", "syntest", "config", 
      "proxy_host: %%(ts1)\nproxy_port: %%(ts1:tsHttpPort)\n");

our @syntest_start_args = 
    ("args", "-f ldap_tests.cfg -c ACC-Basic-Authentication -noquit");

our @empty_args = ();

#
# Traffic Server config + startup
#
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);

## HEADER
if (1) {
   my $proxy_port = TestExec::get_var_value('ts1:tsHttpPort');
   my $proxy_server = TestExec::get_var_value('ts1');

   TestExec::add_to_log("--------------------------------------------------");
   TestExec::add_to_log("Traffic Server HTTP port: $proxy_port");
   TestExec::add_to_log("Traffic Server HTTP host: $proxy_server");
   TestExec::add_to_log(" ");
   TestExec::add_to_log("LDAP Server host: $LDAP_SERVER");
   TestExec::add_to_log("--------------------------------------------------");
   TestExec::add_to_log("Policy Config:");
   foreach my $ln ((split /\n/, $pcfg_text)) {
      TestExec::add_to_log("  $ln");
   }
   TestExec::add_to_log("--------------------------------------------------");
   TestExec::add_to_log("             Starting Tests");
}

# Start TS
TestExec::pm_start_instance("ts1", \@ts_start_args);

# Wait for TS http port to become live
our $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

#
# Syntest config + startup
#
TestExec::pm_create_instance ("syntest1", "%%(load1)", 
			      \@syntest_create_args);


# Install
TestExec::put_instance_file_raw("syntest1", "ldap_tests.cfg",   
				"ldap_tests.cfg");

TestExec::put_instance_file_raw("syntest1", "ldap_syntest.tar", 
				"ldap_syntest.tar");

TestExec::put_instance_file_raw("syntest1", "untar.sh", 
				"../../../plugins/common/untar.sh");

our $result = TestExec::pm_run_slave("syntest1",
				 "untar.sh",
				 "ldap_syntest.tar",
				 10000);
if ($result != 0) {
    TestExec::add_to_log("Error: Failed to install test cases");
#     die;
}

# Now start syntest
TestExec::pm_start_instance("syntest1", \@syntest_start_args);

#
# Test execution
#

sleep(5);

our @raf_args1 = "/processes/syntest1/pid";

sleep(2);

TestExec::add_to_log("Waiting for syntest1 to exit\n");
print "Waiting for syntest1 to exit\n";

my $MAX_SYNTEST_RUN = 30;
my $start_time = time;

# print "START DEBUGGER NOW\n";
# sleep 40;

# Loop waiting for syntest to finish
while (1) {
    my $curr_time = time;

    if (($curr_time - $start_time) > $MAX_SYNTEST_RUN) {
     TestExec::add_to_log("Error: aborting syntest1 instance, timeout");
     last;
    }

    my @raf_result = TestExec::raf_proc_manager("syntest1", "query", 
					     \@raf_args1);

    if (scalar(@raf_result != 3) || $raf_result[0] != 0 || 
	$raf_result[2] < 0) 
    {
	last;
    } else {
	sleep(15);
    }
}

# Clean up
TestExec::pm_stop_instance("syntest1", \@empty_args);
TestExec::pm_destroy_instance("syntest1", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);

TestExec::add_to_log("--------------------------------------------------");
TestExec::add_to_log("             Tests Finished");
TestExec::add_to_log("==================================================");
