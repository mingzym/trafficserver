#
#  misc_time_range2.pl
#
#    Run the default syntest testing suite against Traffic Server
#
#
#  $Id: misc_time_range2.pl,v 1.2 2003-06-01 18:38:32 re1 Exp $
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
add CONFIG proxy.config.diags.debug.enabled INT 0
add CONFIG proxy.config.diags.debug.tags STRING acc.*|policy.*
add CONFIG proxy.config.diags.output.debug STRING SE
add CONFIG proxy.config.core_limit INT -1

[storage.config]

. 700000

[filter.config]

dest_ip=0.0.0.0-255.255.255.255 time=&&(eval|{my \$t=time; my (\$s1,\$m1,\$h1)=localtime(\$t); my (\$s2,\$m2,\$h2)=localtime(\$t); "\$h1:\$m1-100:00"}|) action=deny
EOF

@ts_create_args = ("package", "ts",
		   "config", $ts_config);
$ts_local = TestExec::get_var_value("ts_localpath");
if ($ts_local) {
    print "Using ts_localpath: $ts_local\n";
    push(@ts_create_args, "localpath", $ts_local);
}

# http_syntest
@http_syntest_create_args =
    ("package", "syntest",
     "config", "proxy_host: %%(ts1)\nproxy_port: %%(ts1:tsHttpPort)\n");

@empty_args = ();

# Start up Traffic Server
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_run_slave("ts1","bin/filter_to_policy","",10000);
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for TS ftp port to become live
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start up syntest client
TestExec::pm_create_instance("http_syntest", "%%(load1)",
			     \@http_syntest_create_args);

# Install
TestExec::put_instance_file_raw("http_syntest",
				"time_ranges.tar", "time_ranges.tar");
TestExec::put_instance_file_raw("http_syntest",
				"untar.sh","untar.sh");
$result = TestExec::pm_run_slave("http_syntest",
				 "untar.sh","time_ranges.tar",10000);
if ($result != 0) {
    TestExec::add_to_log("Error: Failed to install test cases");
    die;
}

@http_syntest_start_args = ("args","-f curr_time_ranges.cfg -c CurrTimeRange");
TestExec::pm_start_instance("http_syntest",
			    \@http_syntest_start_args);

sleep(5);

@raf_args1 = "/processes/http_syntest/pid";

TestExec::add_to_log("Waiting for http_syntest to exit\n");
print "Waiting for http_syntest to exit\n";

# Loop waiting for ftp_syntest to finish
while (1) {
    @raf_result = TestExec::raf_proc_manager("http_syntest", "query",
					     \@raf_args1);

    if (scalar(@raf_result != 3) || $raf_result[0] != 0 || $raf_result[2] < 0) {
	last;
    } else {
	sleep(15);
    }
}

# Clean up
TestExec::pm_stop_instance("http_syntest", \@empty_args);
TestExec::pm_destroy_instance("http_syntest", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);

