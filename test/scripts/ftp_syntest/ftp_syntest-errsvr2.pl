#
#  ftp_syntest-errsvr2.pl
#
#    Run the default ftp_syntest testing suite against Traffic Server
#
#
#  $Id: ftp_syntest-errsvr2.pl,v 1.2 2003-06-01 18:38:33 re1 Exp $
#

use TestExec;

$ts_config = <<EOF;
[records.config]

proxy.config.ftp.ftp_enabled          1
proxy.config.ftp.reverse_ftp_enabled  1
proxy.config.diags.output.status      SE
proxy.config.diags.output.note 	      SE
proxy.config.diags.output.warning     SE
proxy.config.diags.output.error       SE
proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name               deft.inktomi.com
proxy.config.hostdb.size              50000
add CONFIG proxy.config.diags.debug.enabled INT 0
add CONFIG proxy.config.diags.debug.tags STRING ftp_protocol
add CONFIG proxy.config.diags.output.debug STRING SE
add CONFIG proxy.config.core_limit INT -1

[storage.config]

. 700000

[ftp_remap.config]
cachedev:&&(proxy.config.ftp.proxy_server_port) cachedev:%%(ftp_syntest_server:server)
EOF
# %%(ts1):&&(proxy.config.ftp.proxy_server_port) %%(load2):%%(ftp_syntest_server:server)

@ts_create_args = ("package", "ts",
		   "config", $ts_config);
$ts_local = TestExec::get_var_value("ts_localpath");
if ($ts_local) {
    print "Using ts_localpath: $ts_local\n";
    push(@ts_create_args, "localpath", $ts_local);
}

# ftp_syntest server
@ftp_syntest_server_create_args =
    ("package", "ftp_syntest",
     "config", "-S -R cachedev");
#     "config", "-S -R %%(load2)");

# ftp_syntest client
@ftp_syntest_client_create_args =
    ("package", "ftp_syntest",
     "config", "-C -P cachedev -p %%(ts1:tsFtpPort) -i %%(ftp_syntest_server:internal)");
#     "config", "-C -P %%(ts1) -p %%(ts1:tsFtpPort) -i %%(ftp_syntest_server:internal)");

@empty_args = ();

# Start up syntest server
TestExec::pm_create_instance("ftp_syntest_server", "%%(load2)",
			     \@ftp_syntest_server_create_args);
# Install
TestExec::put_instance_file_raw("ftp_syntest_server",
				"tests.tar", "tests.tar");
TestExec::put_instance_file_raw("ftp_syntest_server",
				"untar.sh","untar.sh");
$result = TestExec::pm_run_slave("ftp_syntest_server",
				 "untar.sh","tests.tar",10000);
if ($result != 0) {
    TestExec::add_to_log("Error: Failed to install test cases");
    die;
}
TestExec::pm_start_instance("ftp_syntest_server", \@empty_args);

# Start up Traffic Server
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for TS ftp port to become live
#$r = TestExec::wait_for_server_port("ts1", "tsFtpPort", 60000);
#if ($r < 0) {
#    TestExec::add_to_log("Error: TS failed to startup");
#    die "TS failed to start up\n";
#}

# Wait for the ftp_remap.config to become effective
sleep(10);

# Start up syntest client
TestExec::pm_create_instance("ftp_syntest_client", "%%(load1)",
			     \@ftp_syntest_client_create_args);

# Install
TestExec::put_instance_file_raw("ftp_syntest_client",
				"tests.tar", "tests.tar");
TestExec::put_instance_file_raw("ftp_syntest_client",
				"untar.sh","untar.sh");
$result = TestExec::pm_run_slave("ftp_syntest_client",
				 "untar.sh","tests.tar",10000);
if ($result != 0) {
    TestExec::add_to_log("Error: Failed to install test cases");
    die;
}

@ftp_syntest_client_start_args =
    ("args", "-f errsvr2.xml -d .");
TestExec::pm_start_instance("ftp_syntest_client",
			    \@ftp_syntest_client_start_args);

sleep(5);

@raf_args1 = "/processes/ftp_syntest_client/pid";

TestExec::add_to_log("Waiting for ftp_syntest client to exit\n");
print "Waiting for ftp_syntest client to exit\n";

# Loop waiting for ftp_syntest to finish
while (1) {
    @raf_result = TestExec::raf_proc_manager("ftp_syntest_client", "query",
					     \@raf_args1);

    if (scalar(@raf_result != 3) || $raf_result[0] != 0 || $raf_result[2] < 0) {
	last;
    } else {
	sleep(15);
    }
}

# Clean up
TestExec::pm_stop_instance("ftp_syntest_server", \@empty_args);
TestExec::pm_destroy_instance("ftp_syntest_server", \@empty_args);

TestExec::pm_stop_instance("ftp_syntest_client", \@empty_args);
TestExec::pm_destroy_instance("ftp_syntest_client", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);

