#
# A testscript to test selective proxying/caching for all MIXT formats.
#

# add directory that contains script to module includes list
BEGIN {
    use FindBin qw($Bin);
    push(@INC, $Bin);
}

use TestExec;
use MixtUtils;

#
# TS is in forward proxy mode
#
$ts_config = <<EOC;
[records.config]
$MixtUtils::mixt_records_config

[storage.config]
$MixtUtils::mixt_storage_config

[logs_xml.config]
$MixtUtils::mixt_logs_xml_config

[cache.config]
dest_host=%%(qt1) scheme=rtsp tag=QT action=never-cache
dest_host=%%(rn1) scheme=rtsp tag=RNI action=never-cache
dest_host=%%(wm4) scheme=mms action=never-cache
dest_host=%%(ts1) scheme=rtsp action=never-cache

[remap.config]
map rtsp://%%(ts1)/ rtsp://%%(qt1)/ QT
EOC

@empty_args = ();

# Startup traffic server
testcase("1.0) Starting TS", sub {
    @ts_create_args = ("package", "ts",
		       "localpath", "%%(ts_localpath)",
		       "config", $ts_config);
    @ts_start_args = ("args", "-K");

    TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
    TestExec::pm_start_instance("ts1", \@ts_start_args);
    MixtUtils::wait_for_port("ts1", "ts1:rafPort", 60000);    
    $ts1_run_dir = TestExec::get_var_value("ts1:run_dir");
});

testcase("1.1) Starting tsmon for TS", sub {
    @log_reader_create_args = ("package", "tsmon",
			       "config", "$ts1_run_dir deft-mixt");
      
    TestExec::pm_create_instance("tsmon1", "%%(ts1)", \@log_reader_create_args);
    TestExec::pm_start_instance("tsmon1", \@empty_args);

    MixtUtils::wait_for_port("ts1", "tsmon1:rafPort", 60000);
});


testcase("1.2) Starting ffqtload", sub {
    @ffqtload_create_args = ("package", "ffqtload");
    @ffqtload_start_args = ("args", "-printbw");
    TestExec::pm_create_instance("ffqtload1", "%%(load1)", \@ffqtload_create_args);
    TestExec::set_log_parser("ffqtload1", "parse_fftools");
    TestExec::pm_start_instance("ffqtload1", \@ffqtload_start_args);

    MixtUtils::wait_for_port("ffqtload1", "ffqtload1:rafPort", 60000);
});

testcase("1.3) Starting ffwmload2", sub {
    @ffwmload2_create_args = ("package", "ffwmload2");
    @ffwmload2_start_args = ("args", "-printbw");
    TestExec::pm_create_instance("ffwmload1", "%%(load2)", \@ffwmload2_create_args);
    TestExec::set_log_parser("ffwmload1", "parse_fftools");
    TestExec::pm_start_instance("ffwmload1", \@ffwmload2_start_args);

    MixtUtils::wait_for_port("ffwmload1", "ffwmload1:rafPort", 60000);
});

testcase("2.1) Starting load", \&add_clients);

testcase("2.2) Waiting for clients to pull data", sub {
    sleep(45);
});

testcase("2.3) Checking if clients are getting data", \&check_bw);

# Stop clients and restart them; then verify cacheability

testcase("2.4) Stopping clients", \&stop_clients);

testcase("2.5) Waiting for log entries", sub {
    sleep(10);
});

# Verify log entries here: for proxyOnly: prcb = 0
testcase("2.6) Checking log entries", sub {
    @constraints = ("prob > 0", "prcb == 0", "cgid eq \"proxyOnly\"", "cquc =~ /parkwars.mov/",
		    "styp eq \"demand/passthrough-filter\"");
    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for uncacheable parkwars.mov",
			@constraints);

    @constraints = ("prob > 0", "prcb == 0", "cgid eq \"cacheable\"", "cquc =~ /parkwars.mov/",
		    "styp eq \"demand/cached\"");
    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for cacheable parkwars.mov",
			@constraints);    

    @constraints = ("prob > 0", "prcb == 0", "cquc =~ /Vertical_1_31.asf/",
		    "styp eq \"demand/passthrough-filter\"");
    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for uncacheable Vertical_1_31.asf",
			@constraints);    

    @constraints = ("prob > 0", "prcb == 0", "cquc =~ /Vertical_2_31.asf/",
		    "styp eq \"demand/cached\"");
    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for cacheable Vertical_2_31.asf",
			@constraints);    
});

# Restart the load again
testcase ("3.1) Re-starting load", \&add_clients);

testcase("3.2) Waiting for clients to pull data", sub {
    sleep(45);
});

testcase ("3.3) Checking if clients are getting data", \&check_bw);

# Stop clients and restart them; then verify cacheability

testcase ("3.4) Stopping clients", \&stop_clients);

testcase("3.5) Waiting for log entries", sub {
    sleep(10);
});

testcase("3.5) Waiting for log entries", sub {
    @constraints = ("prob > 0", "prcb == 0", "cgid eq \"proxyOnly\"", "cquc =~ /parkwars.mov/",
		    "styp eq \"demand/passthrough-filter\"");
    check_matching_logs("tsmon1", "== 2", "Got [val], expecting [exp] for uncacheable parkwars.mov",
			@constraints);

    @constraints = ("prcb > 0", "cgid eq \"cacheable\"", "cquc =~ /parkwars.mov/",
		    "styp eq \"demand/cached\"");
    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for cacheable parkwars.mov",
			@constraints);

    @constraints = ("prob > 0", "prcb == 0", "cquc =~ /Vertical_1_31.asf/",
		    "styp eq \"demand/passthrough-filter\"");
    check_matching_logs("tsmon1", "== 2", "Got [val], expecting [exp] for uncacheable Vertical_1_31.asf",
			@constraints);    

    @constraints = ("prcb > 0", "cquc =~ /Vertical_2_31.asf/",
		    "styp eq \"demand/cached\"");
    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for cacheable Vertical_2_31.asf",
			@constraints);    
});

#
# Cleanup
#
END {
    testcase("CLEANUP) Cleaning up services", sub {
      MixtUtils::stop_destroy_instance("ffqtload1", @empty_args);
      MixtUtils::stop_destroy_instance("ffwmload1", @empty_args);
      MixtUtils::stop_destroy_instance("tsmon1", @empty_args);
      MixtUtils::stop_destroy_instance("ts1", @empty_args);
    });
}

sub add_clients {
    @raf_args = qw(instance add inst1 rtsp://%%(qt1)/parkwars.mov transport udp count 1 start 1 host %%(ts1) port %%(ts1:tsRtspPort) useragent);
    push(@raf_args, "QTS proxyOnly");
    raf_cmd("ffqtload1", @raf_args);

    @raf_args = qw(instance add inst2 rtsp://%%(qt2)/parkwars.mov transport udp count 1 start 1 host %%(ts1) port %%(ts1:tsRtspPort) useragent);
    @raf_args = (@raf_args, "QTS cacheable");
    raf_cmd("ffqtload1", @raf_args);


    raf_cmd("ffwmload1", qw(instance add inst1 mms://%%(ts1):%%(ts1:tsWmtPort)/ink/rh/%%(wm4)/raw_content/Vertical_1_31.asf transport udp count 1 start 1));

    raf_cmd("ffwmload1", qw(instance add inst2 mms://%%(ts1):%%(ts1:tsWmtPort)/ink/rh/%%(wm2)/Vertical_2_31.asf transport udp count 1 start 1));
}

sub stop_clients {
    raf_cmd("ffqtload1", qw(instance delete inst1));
    raf_cmd("ffqtload1", qw(instance delete inst2));
    raf_cmd("ffwmload1", qw(instance delete inst1));
    raf_cmd("ffwmload1", qw(instance delete inst2));
}

sub check_bw{
    $bw = get_qt_bandwidth("ffqtload1", "inst1");
    if ($bw < 5000) {
      TestExec::add_to_log("Error: qt inst1 isn't getting enough b/w: $bw");
    }

    $bw = get_qt_bandwidth("ffqtload1", "inst2");
    if ($bw < 5000) {
      TestExec::add_to_log("Error: qt inst2 isn't getting enough b/w: $bw");
    }

    $bw = get_wmt_od_bandwidth("ffwmload1", "inst1");
    if ($bw < 5000) {
      TestExec::add_to_log("Error: wmt inst1 isn't getting enough b/w: $bw");
    }

    $bw = get_wmt_od_bandwidth("ffwmload1", "inst2");
    if ($bw < 5000) {
      TestExec::add_to_log("Error: wmt inst2 isn't getting enough b/w: $bw");
    }
}
