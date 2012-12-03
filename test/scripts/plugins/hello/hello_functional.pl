#
#  hello-functional.pl
#
#       This script starts traffic server with hello plugin loaded and the 
#       debug tag "debug-hello" activated. The plugin will output "Hello 
#       World!" which should be present in output log file.
#
#   Functional testing for plugin hello
#
#  Author: bdoshi
#
#  $Id: 
#

use TestExec;
use ConfigHelper;

# TS configuration
$cfg = new ConfigHelper;
# $cfg->set_debug_tags ('debug-hello.*');
$cfg->add_config('plugin.config');
$cfg->add_config_line ('hello.so');
$ts_config = $cfg->output;



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


# Install the hello plugin
$plg_src = "../../../../proxy/api/samples/hello/";
$plg_dst = "config/plugins/";
$cfg_src = "config/";
$cfg_dst = "config/plugins/";
TestExec::put_instance_file_raw("ts1", $plg_dst . "hello.so", $plg_src . "hello.so");

# Startup the Traffic Server Instance
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for the http port to becom live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Kill Traffic Server
TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);





