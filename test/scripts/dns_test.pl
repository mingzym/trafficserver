use TestExec;

$ts_config = <<EOF;
[records.config]

proxy.config.diags.output.status      SE
proxy.config.diags.output.note 	      SE
proxy.config.diags.output.warning     SE
proxy.config.diags.output.error       SE
proxy.config.cache.storage_filename   storage.config
proxy.config.core_limit		      -1
proxy.config.hostdb.size              50000
proxy.config.icp.icp_port	      9228
proxy.config.log2.max_entries_per_buffer	5
proxy.config.snmp.master_agent_enabled   0
proxy.config.http_ui_enabled 2
proxy.config.http.push_method_enabled 1
proxy.config.log2.search_log_enabled 0
proxy.config.http.insert_request_via_str 1
proxy.config.http.insert_response_via_str 1


[storage.config]


. 700000
EOF

@ts_create_args = ("package", "ts",
 	           "localpath", "%%(ts_localpath)",
		   "config", $ts_config);

@jtest_create_args = ("package", "jtest",
		      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 1");

@jtest_start_args = ("args", "-C");
@empty_args = ();

@livetest_create_args = ("package", "livetest",
			 "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 1 -u livetest.log");

@livetest_start_args = ("args", "-C");

$ts_inst_name = "ts1";
TestExec::pm_create_instance($ts_inst_name, "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance($ts_inst_name, \@empty_args);


# Wait for the http port to becom live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

TestExec::pm_create_instance("jtest1", "%%(load1)", \@jtest_create_args);
TestExec::pm_start_instance("jtest1", \@jtest_start_args);

TestExec::pm_create_instance("livetest1", "%%(load1)", \@livetest_create_args);
TestExec::pm_start_instance("livetest1", \@livetest_start_args);

sleep(5);

@raf_args1 = "/stats/client_sec";
@raf_args2 = "/processes/ts1/pid";

for ($i = 0; $i < 60; $i++) {
    @raf_result = TestExec::raf_instance("jtest1", "query", \@raf_args1);
    TestExec::add_to_log("Raf Result: $raf_result[0] $raf_result[1] $raf_result[2]");

    @raf_result = TestExec::raf_proc_manager("ts1", "query", \@raf_args2);
    TestExec::add_to_log("PM Raf Result: $raf_result[0] $raf_result[1] $raf_result[2]");
    sleep(2);
}

TestExec::pm_stop_instance("jtest1", \@empty_args);
TestExec::pm_destroy_instance("jtest1", \@empty_args);

TestExec::pm_stop_instance("livetest1", \@empty_args);
TestExec::pm_destroy_instance("livetest1", \@empty_args);

TestExec::pm_stop_instance($ts_inst_name, \@empty_args);
TestExec::pm_destroy_instance($ts_inst_name, \@empty_args);













