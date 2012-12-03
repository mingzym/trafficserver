#
#  bnull_functional.pl (based on append_function by frackc)
#
#    Run functional test cases for the buffered null transform plugin
#
#  Author: bevans
#
#  $Id: bnull_functional.pl,v 1.2 2003-06-01 18:38:33 re1 Exp $
#

use TestExec;
use ConfigHelper;

# TS configuration
$cfg = new ConfigHelper;
# $cfg->set_debug_tags ('http_hdrs|policy.*');
$cfg->add_config('plugin.config');
$cfg->add_config_line ('bnull-transform.so');
$ts_config = $cfg->output;


@syntest_create_args = 
    ( "package", "syntest", "config", 
      "proxy_host: %%(ts1)\nproxy_port: %%(ts1:tsHttpPort)\n");

@syntest_start_args = 
    ("args", "-f null_tests.cfg -c Null-Transform -noquit");

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
$plg_src = "../../../../proxy/api/samples/bnull-transform/";
$plg_dst = "config/plugins/";
$cfg_src = "config/";
$cfg_dst = "config/plugins/";

TestExec::put_instance_file_raw
    ("ts1", $plg_dst . "bnull-transform.so",  
     $plg_src . "bnull-transform.so",);

# TestExec::put_instance_file_raw
#     ("ts1", $cfg_dst . 
#      "append-this-file.txt", $cfg_src . "append-this-file.txt");

# Start TS
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for TS http port to become live
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
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
TestExec::put_instance_file_raw("syntest1", "null_tests.cfg",   
				"null_tests.cfg");

# unlink 'null_syntest.tar';
# system q(tar cf null_syntest.tar Tests);

TestExec::put_instance_file_raw("syntest1", "null_syntest.tar", 
				"null_syntest.tar");

TestExec::put_instance_file_raw("syntest1", "untar.sh", 
				"../common/untar.sh");

# Note: pm_run_slave change dir to the run_dir of instance,
# then execute the command specified in arguments.
$result = TestExec::pm_run_slave("syntest1",
				 "untar.sh",
				 "null_syntest.tar",
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

@raf_args1 = "/processes/syntest1/pid";

TestExec::add_to_log("Waiting for syntest1 to exit\n");
print "Waiting for syntest1 to exit\n";

# Loop waiting for syntest to finish
while (1) {
    @raf_result = TestExec::raf_proc_manager("syntest1", "query", 
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
