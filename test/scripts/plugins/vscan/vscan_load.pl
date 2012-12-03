#
#  vscan.pl
#
#  Virus Scanning plugin test
#
# Author: franckc
#

use TestExec;
use ConfigHelper;

# TS configuration
$cfg = new ConfigHelper;
# $cfg->set_debug_tags ('vscan.*');
$cfg->add_config('plugin.config');
$cfg->add_config_line ('vscan.so');
$ts_config = $cfg->output;


@ts_start_args = ("args", "-k -K");

@jtest_create_args = ("package", "jtest",
                      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -S %%(load1) -c 1");

@jtest1_start_args = ("args", "--nocheck_length -g 0.1 -g 0.1");

@jtest2_start_args = ("args", "--nocheck_length -g 0.1 --ftp --ftp_mdtm_err_rate 0.05 --ftp_mdtm_rate 0.05 -z 0.4");



@loadgen_create_args = ("package", "loadgen_http",
                      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -S %%(load2) -c 10");

@loadgen_start_args = ("args", "-t testplans/telenor.xml");

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

# Install the vscan plugin
$plg_src = "../../../../proxy/api/integration/VirusScan/";
$plg_dst = "config/plugins/";
$cfg_src = "config/";
$cfg_dst = "config/plugins/";
TestExec::put_instance_file_raw("ts1", $plg_dst . "vscan.so",            $plg_src . "vscan.so",);
TestExec::put_instance_file_raw("ts1", $cfg_dst . "extensions.config",   $cfg_src . "extensions.config");
TestExec::put_instance_file_raw("ts1", $cfg_dst . "trusted-host.config", $cfg_src . "trusted-host.config");
TestExec::put_instance_file_raw("ts1", $cfg_dst . "trusted-type.config", $cfg_src . "trusted-type.config");

# pretty cool: vscan.config contains a reference to %%(css_server_ip)
# that will be substituted by the value defined below when file is copied !
TestExec::set_var_value("css_server_ip", "Server:216.155.193.72:7777");
TestExec::put_instance_file_subs("ts1", $cfg_dst . "vscan.config",        $cfg_src . "vscan.config");

# Start TS
TestExec::pm_start_instance("ts1", \@ts_start_args);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}


#
# Start the load
#

# Start the jtest instances
TestExec::pm_create_instance("jtest1", "%%(load1)", \@jtest_create_args);
TestExec::pm_start_instance("jtest1", \@jtes1t_start_args);
TestExec::add_to_log("JTEST instance 1 started successfully");

TestExec::pm_create_instance("jtest2", "%%(load1)", \@jtest_create_args);
TestExec::pm_start_instance("jtest2", \@jtest2_start_args);
TestExec::add_to_log("JTEST instance 2 started successfully");


# Start the loadgen instances
TestExec::pm_create_instance("loadgen1", "%%(load2)", \@loadgen_create_args);
TestExec::set_log_parser("loadgen1", "parse_loadgen");
TestExec::pm_start_instance("loadgen1", \@loadgen_start_args);
TestExec::add_to_log("LOADGEN instance 1 started successfully");



# Keep test running for a certain period of time
sleep(30);


#
# Clean up
#
TestExec::pm_stop_instance("jtest1", \@empty_args);
TestExec::pm_destroy_instance("jtest1", \@empty_args);

TestExec::pm_stop_instance("jtest2", \@empty_args);
TestExec::pm_destroy_instance("jtest2", \@empty_args);

TestExec::pm_stop_instance("loadgen1", \@empty_args);
TestExec::pm_destroy_instance("loadgen1", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);


# Check for any core dump ???
# Check memory leak ???





