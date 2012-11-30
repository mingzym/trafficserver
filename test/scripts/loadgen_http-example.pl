#
# loadgen-example.pl
#
# Sample loadgen test for DEFT
#
# Author: franckc
#

use TestExec;

$ts_config = <<EOC;
[records.config]

add CONFIG proxy.config.diags.output.debug       STRING S
add CONFIG proxy.config.diags.output.status      STRING S
add CONFIG proxy.config.diags.output.note        STRING S
add CONFIG proxy.config.diags.output.warning     STRING E
add CONFIG proxy.config.diags.output.error       STRING E
add CONFIG proxy.config.diags.output.fatal       STRING E 
add CONFIG proxy.config.diags.output.alert       STRING E
add CONFIG proxy.config.diags.output.emergency   STRING E

proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name               vscan.inktomi.com
proxy.config.hostdb.size              200000
add CONFIG proxy.config.core_limit INT -1

[storage.config]

. 64000000
EOC

@ts_create_args = ("package", "ts",
                   "config", $ts_config);

@ts_start_args = ("args", "-k -K");


@loadgen_create_args1 = ("package", "loadgen_http",
                      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -S %%(load1) -c 10");


@loadgen_create_args2 = ("package", "loadgen_http",
                      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -S %%(load2) -c 10");

@loadgen_start_args1 = ("args", "-t testplans/mitsubishi.xml");

@loadgen_start_args2 = ("args", "-t testplans/telenor.xml");


@empty_args = ();

# Start up the Traffic Server instance
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@ts_start_args);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the loadgen instances
TestExec::set_log_parser("loadgen1", "parse_loadgen");
TestExec::pm_create_instance("loadgen1", "%%(load1)", \@loadgen_create_args1);
TestExec::pm_start_instance("loadgen1", \@loadgen_start_args1);
TestExec::add_to_log("LOADGEN instance 1 started successfully");

TestExec::set_log_parser("loadgen2", "parse_loadgen");
TestExec::pm_create_instance("loadgen2", "%%(load2)", \@loadgen_create_args2);
TestExec::pm_start_instance("loadgen2", \@loadgen_start_args2);
TestExec::add_to_log("LOADGEN instance 2 started successfully");


# Keep test running for 60sec
sleep(60);


# Query loadgen via raf interface to get stats
@raf_args1 = "/stat/client/ops_total";
@raf_result1 = TestExec::raf_instance("loadgen1", "query", \@raf_args1);

@raf_args2 = "/stat/server/ops_total";
@raf_result2 = TestExec::raf_instance("loadgen1", "query", \@raf_args1);

# Must have done at least 50 operations on client and server
if (($raf_result1[2] < 50) or ($raf_result2[2] < 50)) {
   TestExec::add_to_log("Error: insufficent loadgen ops - client: " . $raf_result1[2] . " server: " . $raf_result2[2]);
} else {
   TestExec::add_to_log("Status: loadgen ops - client: " . $raf_result1[2] . " server: " . $raf_result2[2]);
}


# Stop the load and TS
TestExec::pm_stop_instance("loadgen1", \@empty_args);
TestExec::pm_destroy_instance("loadgen1", \@empty_args);

TestExec::pm_stop_instance("loadgen2", \@empty_args);
TestExec::pm_destroy_instance("loadgen2", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);

