#
#  api-regression_functional.pl
#
#    Run functional test cases for internal QA regression
#    plugin api-regression
#
#    At startup, the plugin tests correctness of SDK API constants.
#    Then it registers to all the SDK HTTP hooks.
#    Plugin asserts if any error detected.
#
#    Testing is done by using jtest.
#    Note: should make sure we exercise all HTTP hooks...
#
#
#  Author: franckc
#
#  $Id:
#

use TestExec;
use ConfigHelper;

# TS configuration
$cfg = new ConfigHelper;
# $cfg->set_debug_tags ('api-regression');
$cfg->add_config('plugin.config');
$cfg->add_config_line ('api-regression.so');
$cfg->add_record ('INT', 'proxy.config.core_limit', '-1');
$ts_config = $cfg->output;


@jtest_create_args = ("package", "jtest",
                      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 5");

#@jtest_start_args = ("args", "-N 3");
@jtest_start_args = ("args", "");

@empty_args = ();


#
# Traffic Server config + startup
#
@ts_create_args = ("package", "ts",
                   "config", $ts_config);

# Check to see if we are using localpath ts
$ts_local = TestExec::get_var_value("ts_localpath");
if ($ts_local) {
    print "Using ts_localpath: $ts_local\n";
    push(@ts_create_args, "localpath", $ts_local);
}

TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);


# Install the plugin
$plg_src = "../../../../proxy/api/samples/regression/api-regression/";
$plg_dst = "config/plugins/";
$cfg_src = "config/";
$cfg_dst = "config/plugins/";

TestExec::put_instance_file_raw
    ("ts1", $plg_dst . "api-regression.so",  
     $plg_src . "api-regression.so",);


# Start TS
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for TS http port to become live
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}


# Start the jtest instance
TestExec::pm_create_instance("jtest1", "%%(load1)", \@jtest_create_args);
TestExec::pm_start_instance("jtest1", \@jtest_start_args);

#
# Test execution
#

# keep test running for 60 secs
sleep(60);

# make sure at jtest was processing request
@raf_args1 = "/stats/client_sec";
@raf_result = TestExec::raf_instance("jtest1", "query", \@raf_args1);

if ( scalar(@raf_result != 3) || $raf_result[0] != 0 || $raf_result[2] < 1) {
  TestExec::add_to_log("ERROR: insufficent jtest client ops: $raf_result[2]"); 
}
else {
  TestExec::add_to_log("Status: jtest ops: $raf_result[2]"); 
}


#
# Clean up
#
TestExec::pm_stop_instance("jtest1", \@empty_args);
TestExec::pm_destroy_instance("jtest1", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);
