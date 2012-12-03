#
#  syntest.pl
#
#    Run the default syntest testing suite against Traffic Server
#
#
#  $Id: http_syntest.pl,v 1.2 2003-06-01 18:38:33 re1 Exp $
#

use TestExec;

$ts_config = <<EOF;
[records.config]

proxy.config.diags.output.status      SE
proxy.config.diags.output.note 	      SE
proxy.config.diags.output.warning     SE
proxy.config.diags.output.error       SE
proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name               deft.inktomi.com
proxy.config.hostdb.size              50000
add CONFIG proxy.config.core_limit INT -1

[storage.config]


. 700000
EOF

@ts_create_args = ("package", "ts",
		   "config", $ts_config);
$ts_local = TestExec::get_var_value("ts_localpath");
if ($ts_local) {
    print "Using ts_localpath: $ts_local\n";
    push(@ts_create_args, "localpath", $ts_local);
}

@syntest_create_args = ("package", "syntest",
		      "config", "proxy_host: %%(ts1)\nproxy_port: %%(ts1:tsHttpPort)\n");

@syntest_start_args = ("args", "-c Trace");
@empty_args = ();

# Start up Traffic Server
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for TS http port to become live
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start up syntest
TestExec::pm_create_instance("syntest1", "%%(load1)", \@syntest_create_args);
#TestExec::pm_start_instance("syntest1", \@syntest_start_args);
TestExec::put_instance_file_raw("syntest1","tests.cfg","tests.cfg");
TestExec::pm_start_instance("syntest1", \@empty_args);

sleep(5);

@raf_args1 = "/processes/syntest1/pid";

TestExec::add_to_log("Waiting for syntest1 to exit\n");
print "Waiting for syntest1 to exit\n";

# Loop waiting for syntest to finish
while (1) {
    @raf_result = TestExec::raf_proc_manager("syntest1", "query", \@raf_args1);

    if (scalar(@raf_result != 3) || $raf_result[0] != 0 || $raf_result[2] < 0) {
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

