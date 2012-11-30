# add directory that contains script to module includes list
BEGIN {
    use FindBin qw($Bin);
    push(@INC, $Bin);
}

use TestExec;
use MixtUtils;

# TS configuration info

###
### parent configuration (forward proxy)
###
$ts_parent_config = <<EOC;
[records.config]
$MixtUtils::mixt_records_config

[storage.config]
$MixtUtils::mixt_storage_config

[logs_xml.config]
$MixtUtils::mixt_logs_xml_config
EOC


###
### child configuration (reverse proxy)
###
$ts_child_config = <<EOC;
[records.config]
$MixtUtils::mixt_records_config
proxy.config.http.parent_proxy_routing_enable      1
proxy.config.reverse_proxy.enabled                 1

[storage.config]
$MixtUtils::mixt_storage_config

[parent.config]

dest_domain=. parent="%%(ts2):%%(ts2:tsWmtPort)" scheme=mms
dest_domain=. parent="%%(ts2):%%(ts2:tsRtspPort)" scheme=rtsp tag=QT

[remap.config]
map rtsp://%%(ts1):&&(proxy.config.mixt.rtsp_proxy_port)/ rtsp://%%(qt1)/ QT
reverse_map rtsp://%%(qt1)/ rtsp://%%(ts1):&&(proxy.config.mixt.rtsp_proxy_port)/ QT

map mms://%%(ts1):&&(proxy.config.wmt.port)/ mms://%%(wm3)/
reverse_map mms://%%(wm3)/ mms://%%(ts1):&&(proxy.config.wmt.port)/

[logs_xml.config]
$MixtUtils::mixt_logs_xml_config

EOC

# shortcuts
@empty_args = ();

###
### test cases
### 

# parent *must* be instantiated first, because child's parent.config rules
# depend on parent's port numbers
testcase("1.0) Starting parent forward proxy TS", sub {
    @ts_parent_create_args = ("package", "ts",
			      "localpath", "%%(ts_localpath)",
			      "config", $ts_parent_config);

    TestExec::pm_create_instance("ts2", "%%(ts2)", \@ts_parent_create_args);
    TestExec::pm_start_instance("ts2", \@ts_start_args);
    MixtUtils::wait_for_port("ts2", "ts2:rafPort", 60000);    
    $ts2_run_dir = TestExec::get_var_value("ts2:run_dir");
});

testcase("1.1) Starting tsmon for parent TS", sub {
    @log_reader_create_args = ("package", "tsmon",
			       "config", "$ts2_run_dir deft-mixt");
      
    TestExec::pm_create_instance("tsmon2", "%%(ts2)", \@log_reader_create_args);
    TestExec::pm_start_instance("tsmon2", \@empty_args);

    MixtUtils::wait_for_port("ts2", "tsmon2:rafPort", 60000);
});

testcase("1.2) Starting child reverse proxy TS", sub {
    @ts_child_create_args = ("package", "ts",
			     "localpath", "%%(ts_localpath)",
			     "config", $ts_child_config);

    TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_child_create_args);
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    MixtUtils::wait_for_port("ts1", "ts1:tsRtspPort", 60000);
    $ts1_run_dir = TestExec::get_var_value("ts1:run_dir");
});

testcase("1.3) Starting tsmon for child TS", sub {
    @log_reader_create_args = ("package", "tsmon",
			       "config", "$ts1_run_dir deft-mixt");
      
    TestExec::pm_create_instance("tsmon1", "%%(ts1)", \@log_reader_create_args);
    TestExec::pm_start_instance("tsmon1", \@empty_args);

    MixtUtils::wait_for_port("ts1", "tsmon1:rafPort", 60000);
});

	 
testcase("1.4) Starting ffqtload", sub {
    @ffqtload_create_args = ("package", "ffqtload");
    @ffqtload_start_args = ("args", "-printbw");
    TestExec::pm_create_instance("ffqtload1", "%%(load1)", \@ffqtload_create_args);
    TestExec::set_log_parser("ffqtload1", "parse_fftools");
    TestExec::pm_start_instance("ffqtload1", \@ffqtload_start_args);

    MixtUtils::wait_for_port("ffqtload1", "ffqtload1:rafPort", 60000);
});

testcase("1.5) Starting ffwmload2", sub {
    @ffwmload2_create_args = ("package", "ffwmload2");
    @ffwmload2_start_args = ("args", "-printbw");
    TestExec::pm_create_instance("ffwmload1", "%%(load2)", \@ffwmload2_create_args);
    TestExec::set_log_parser("ffwmload1", "parse_fftools");
    TestExec::pm_start_instance("ffwmload1", \@ffwmload2_start_args);

    MixtUtils::wait_for_port("ffwmload1", "ffwmload1:rafPort", 60000);
});


testcase("2.1) Starting QT load", sub {
    raf_cmd("ffqtload1", qw(instance add inst1 rtsp://%%(ts1):%%(ts1:tsRtspPort)/parkwars.mov transport udp count 1 start 1));
    raf_cmd("ffqtload1", qw(instance add inst2 rtsp://%%(qt2)/dino.mov transport udp count 1 start 1 host %%(ts2) port %%(ts2:tsRtspPort)));
    raf_cmd("ffqtload1", qw(instance add inst3 rtsp://%%(ts1):%%(ts1:tsRtspPort)/live_300.sdp transport udp count 1 start 1));
    raf_cmd("ffqtload1", qw(instance add inst4 rtsp://%%(qt1)/live_300.sdp transport udp count 2 joinrate 1 start 1 host %%(ts2) port %%(ts2:tsRtspPort)));
});


testcase("2.2) Starting WMT load", sub {
    raf_cmd("ffwmload1", qw(instance add inst1 mms://%%(ts1):%%(ts1:tsWmtPort)/Vertical_1_31.asf transport udp count 1 start 1));
    raf_cmd("ffwmload1", qw(instance add inst2 mms://%%(ts2/r):%%(ts2:tsWmtPort)/ink/rh/%%(wm1/r)/raw_content/Vertical_2_31.asf transport udp count 1 start 1));
});

testcase("2.3) Waiting for clients to pull data", sub {
    sleep(45);
});

testcase("2.4) Checking QT client bandwidth", sub {
    $bw = get_qt_bandwidth("ffqtload1", "inst1");
    if ($bw < 5000) {
	TestExec::add_to_log("Error: qt inst1 isn't getting enough b/w: $bw");
    }

    $bw = get_qt_bandwidth("ffqtload1", "inst2");
    if ($bw < 5000) {
	TestExec::add_to_log("Error: qt inst2 isn't getting enough b/w: $bw");
    }

    $bw = get_qt_bandwidth("ffqtload1", "inst3");
    if ($bw < 5000) {
	TestExec::add_to_log("Error: qt inst3 isn't getting enough b/w: $bw");
      }
    
    $bw = get_qt_bandwidth("ffqtload1", "inst4");
    if ($bw < 5000) {
	TestExec::add_to_log("Error: qt inst3 isn't getting enough b/w: $bw");
    }
});

# disable until IP address stuff is resolved...
testcase("2.5) Checking WMT client bandwidth", sub {
    if (0) {
	$bw = get_wmt_od_bandwidth("ffwmload1", "inst1");
	if ($bw < 5000) {
	    TestExec::add_to_log("Error: wmt inst1 isn't getting enough b/w: $bw");
	  }
    }

    $bw = get_wmt_od_bandwidth("ffwmload1", "inst2");
    if ($bw < 5000) {
	TestExec::add_to_log("Error: wmt inst2 isn't getting enough b/w: $bw");
    }
});

testcase("2.6) Stopping clients", sub {
    raf_cmd("ffqtload1", qw(instance delete inst1));
    raf_cmd("ffqtload1", qw(instance delete inst2));
    raf_cmd("ffqtload1", qw(instance delete inst3));
    raf_cmd("ffqtload1", qw(instance delete inst4));
    raf_cmd("ffwmload1", qw(instance delete inst1));
    raf_cmd("ffwmload1", qw(instance delete inst2));
});

testcase("2.7) Waiting for log entries", sub {
    sleep(10);
});

testcase("2.8) Checking child's log entries", sub {
    @constraints = ("prob > 0", "prcb == 0", "phr eq DEFAULT_PARENT");

    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for parkwars.mov on child!",
			@constraints, "cquc =~ /parkwars.mov/");
    

    check_matching_logs("tsmon1", "== 1", "Got [val], expecting [exp] for live_300.sdp on child!",
			@constraints, "cquc =~ /live_300.sdp/");

    });

testcase("2.8) Checking parent's log entries", sub {
    @constraints = ("prob > 0", "prcb == 0", "phr eq NONE");

    check_matching_logs("tsmon2", "== 3", "Got [val], expecting [exp] for live_300.sdp on parent!",
			@constraints, "cquc =~ /live_300.sdp/");

    check_matching_logs("tsmon2", "== 1", "Got [val], expecting [exp] for parkwars.mov on parent!",
			@constraints, "cquc =~ /parkwars.mov/");
    
    check_matching_logs("tsmon2", "== 1", "Got [val], expecting [exp] for dino.mov on parent!",
			@constraints, "cquc =~ /dino.mov/");

    check_matching_logs("tsmon2", "== 1", "Got [val], expecting [exp] for Vertical_2_31.asf on parent!",
			@constraints, "cquc =~ /Vertical_2_31.asf/");
});

#
# cleanup
#

END {
    testcase("CLEANUP) Cleaning up services", sub {
	# ffqtload stopped above
        TestExec::pm_stop_instance("ffqtload1", \@empty_args);
	TestExec::pm_destroy_instance("ffqtload1", \@empty_args);

        TestExec::pm_stop_instance("ffwmload1", \@empty_args);
	TestExec::pm_destroy_instance("ffwmload1", \@empty_args);
	  
        TestExec::pm_stop_instance("tsmon1", \@empty_args);
	TestExec::pm_destroy_instance("tsmon1", \@empty_args);

        TestExec::pm_stop_instance("tsmon2", \@empty_args);
	TestExec::pm_destroy_instance("tsmon2", \@empty_args);

	TestExec::pm_stop_instance("ts1", \@empty_args);
	TestExec::pm_destroy_instance("ts1", \@empty_args);

	TestExec::pm_stop_instance("ts2", \@empty_args);
	TestExec::pm_destroy_instance("ts2", \@empty_args);
    });
}
