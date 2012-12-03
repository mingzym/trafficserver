#
#  check-http-0_functional.pl
#
#    Run functional test cases for internal QA regression
#    plugin check-http-0
#
#    The plugin exercises request and response headers
#    In order to feed it with various and erroneous headers,
#    we use loadgen load with a testplan generated from customers logs
#    Plugin asserts if any error detected.
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
# $cfg->set_debug_tags ('check-http-0');
$cfg->add_config('plugin.config');
$cfg->add_config_line ('check-http-0.so');
$cfg->add_record ('INT', 'proxy.config.core_limit', '-1');
$ts_config = $cfg->output;


@loadgen_create_args1 = ("package", "loadgen_http",
			 "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -S %%(load1) -c 10");

@loadgen_start_args1 = ("args", "-t testplans/mitsubishi.xml");

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
$plg_src = "../../../../proxy/api/samples/internal/check-http/";
$plg_dst = "config/plugins/";
$cfg_src = "config/";
$cfg_dst = "config/plugins/";

TestExec::put_instance_file_raw
    ("ts1", $plg_dst . "check-http-0.so",  
     $plg_src . "check-http-0.so",);


# Start TS
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for TS http port to become live
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}


# Start the loadgen instances
TestExec::set_log_parser("loadgen1", "parse_loadgen");
TestExec::pm_create_instance("loadgen1", "%%(load1)", \@loadgen_create_args1);
TestExec::pm_start_instance("loadgen1", \@loadgen_start_args1);
TestExec::add_to_log("LOADGEN instance 1 started successfully");



#
# Test execution
#


#
# Wait until loadgen has done at least 1,000 ops on client side
#
$nb_ops_client = 0;
while ($nb_ops_client < 1000) {

    sleep(10);

    @raf_args1 = "/stat/client/ops_total";
    @raf_result1 = TestExec::raf_instance("loadgen1", "query", \@raf_args1);
    @raf_args2 = "/stat/server/ops_total";
    @raf_result2 = TestExec::raf_instance("loadgen1", "query", \@raf_args1);

    if ( scalar(@raf_result1 != 3) || $raf_result1[0] != 0 || $raf_result1[2] < 0) {
        TestExec::add_to_log("ERROR: loadgen reported suspicious client side ops");
	last;
    }
    if (scalar(@raf_result2 != 3) || $raf_result2[0] != 0 || $raf_result2[2] < 0) {
	TestExec::add_to_log("ERROR: loadgen reported suspicious server side ops");
	last;
    }
    
    $nb_ops_client = $raf_result1[2];
    $nb_ops_server = $raf_result2[2];

    TestExec::add_to_log("Status: client ops = " . $nb_ops_client . " server ops = " . $nb_ops_server);
}


#
# Clean up
#
TestExec::pm_stop_instance("loadgen1", \@empty_args);
TestExec::pm_destroy_instance("loadgen1", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);
