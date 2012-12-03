use TestExec;
use ConfigHelper;
use strict vars;

# TS configuration
our $cfg = new ConfigHelper;
    $cfg->set_debug_tags ('sdk_ut');

our $ts_config = $cfg->output;
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
our @ts_start_args = ("args", "-R 3 -r SDK.\*");

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



