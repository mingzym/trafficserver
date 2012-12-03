use TestExec;
use strict vars;

our $ts_config = <<EOF;
[records.config]

proxy.config.diags.output.status      SE
proxy.config.diags.output.note 	      SE
proxy.config.diags.output.warning     SE
proxy.config.diags.output.error       SE
proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name		      mike-proxy.inktomi.com
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

. 671088640

EOF

our $ts_inst_name = "ts1";

sub wait_for_ts_exit {
    print "Waiting for ts1 to exit\n";
    my @raf_args1 = "/processes/ts1/pid";
    while (1) {
	my @raf_result = TestExec::raf_proc_manager($ts_inst_name, "query", \@raf_args1);

	if (scalar(@raf_result != 3) || $raf_result[0] != 0 || $raf_result[2] < 0) {
	    last;
	} else {
	    sleep(15);
	}
    }
}

our @ts_create_args = ("package", "ts",
		       "localpath", "%%(ts_localpath)",
		       "config",
		       $ts_config);

our @ts_cache_clear = ("args", "-C clear");
our @ts_start_args = ("args", "-R 3");

our @empty_args = ();


TestExec::set_log_parser($ts_inst_name, "parse_ts_regress");
TestExec::pm_create_instance($ts_inst_name, "%%(ts1)", \@ts_create_args);

print "Clearing Cache\n";
TestExec::pm_start_instance($ts_inst_name, \@ts_cache_clear);
wait_for_ts_exit();

print "Starting TS Regression\n";
TestExec::pm_start_instance($ts_inst_name, \@ts_start_args);
wait_for_ts_exit();

TestExec::pm_stop_instance($ts_inst_name, \@empty_args);
TestExec::pm_destroy_instance($ts_inst_name, \@empty_args);



