#
#  time-ranges.pl (based on append_function by frackc)
#
#    Run functional test cases for testing policy engine time ranges
#
#  $Author: re1 $
#
#  $Id: time-ranges.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
#

use TestExec;
use ConfigHelper;
use PolicyConfig;
use strict;

my $CVS_ID_TAG   = '$Id: time-ranges.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $';
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
    unlink 'time-ranges.tar';
    system q(tar cf time-ranges.tar Tests);
    die "chdir failed" unless chdir $deft_path;
}

# TS configuration
our $cfg = new ConfigHelper;
$cfg->set_debug_tags  ('http_hdrs|http_auth|policy.*');
$cfg->set_action_tags ('deft.*');
our $ts_config = $cfg->output;

# Generate a simple Policy_Config.XML.
our $pcfg = new PolicyConfig;
GEN_CONFIG ($pcfg);
our $pcfg_text = $pcfg->config;
$ts_config .= $pcfg_text;

our @ts_create_args = 
    ("package", "ts", "localpath", 
     "%%(ts_localpath)", "config", $ts_config);

our @ts_start_args = ("args", "-Kkj");

our @syntest_create_args = 
    ( "package", "syntest", "config", 
      "proxy_host: %%(ts1)\nproxy_port: %%(ts1:tsHttpPort)\n");

our @syntest_start_args = 
    ("args", "-f time_ranges.cfg -c Policy-TimeRanges -noquit");

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
TestExec::put_instance_file_raw("syntest1", "time_ranges.cfg",   
				"time_ranges.cfg");

TestExec::put_instance_file_raw("syntest1", "time-ranges.tar", 
				"time-ranges.tar");

TestExec::put_instance_file_raw("syntest1", "untar.sh", 
				"../../../plugins/common/untar.sh");

our $result = TestExec::pm_run_slave("syntest1",
				 "untar.sh",
				 "time-ranges.tar",
				 10000);
if ($result != 0) {
    TestExec::add_to_log("Error: Failed to install test cases");
    die;
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

sub GEN_CONFIG
{
  # scripts/perl_lib/PolicyConfig.pm has an example of how to 
  # set this up after the END tag.

  my $cfg = shift;

  my @now = localtime(time);
  my @np8 = localtime(time + (3600 * 8));
  my @np2 = localtime(time + (3600 * 2));
  my @nm8 = localtime(time - (3600 * 8));
  my @nm2 = localtime(time - (3600 * 2));
  
  my $c_any = $cfg->CRITERIA (
         type   => 'dest_domain',
         method => 'domain',
         value  => '.');
 
  my $c_min8 = $cfg->CRITERIA (
         type   => 'time',
         method => 'range',
         value  => qq($nm8[2]:$nm8[1]-$nm2[2]:$nm2[1]));

  my $c_mid8 = $cfg->CRITERIA (
         type   => 'time',
         method => 'range',
         value  => qq($nm2[2]:$nm2[1]-$np2[2]:$np2[1]));

  my $c_plu8 = $cfg->CRITERIA (
         type   => 'time',
         method => 'range',
         value  => qq($np2[2]:$np2[1]-$np8[2]:$np8[1]));

  my $r_any  = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: fall-through"] );
  my $r_min8 = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: past"] );
  my $r_plu8 = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: future"] );
  my $r_mid8 = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: current"] );

  $cfg->KEY ('past', $c_min8);
  $cfg->KEY ('future', $c_plu8);
  $cfg->KEY ('current', $c_mid8);
  $cfg->KEY ('fall-through',  $c_any);

  my $r_min8 = $cfg->RULE (
         keyId => 'past',
         auzn  => 'allow-cfg',
         ruleData => $r_min8);

  my $r_mid8 = $cfg->RULE (
         keyId => 'current',
         auzn  => 'allow-cfg',
         ruleData => $r_mid8);

  my $r_plu8 = $cfg->RULE (
         keyId => 'future',
         auzn  => 'allow-cfg',
         ruleData => $r_plu8);

  my $r_any = $cfg->RULE (
         keyId => 'fall-through',
         auzn  => 'allow-cfg',
         ruleData => $r_any);

  $cfg->ACL ("TE", $r_min8, $r_mid8, $r_plu8, $r_any);
};
