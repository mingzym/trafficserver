#
#  sdktest-example.pl
#
#    Run the default sdktest testing suite against Traffic Server
#
# Author: franckc
#

use TestExec;

$ts_config = <<EOF;
[records.config]

proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name               deft.inktomi.com
proxy.config.hostdb.size              50000

#add CONFIG proxy.config.diags.debug.enabled      INT    1
#add CONFIG proxy.config.diags.debug.tags         STRING <my_debug_tag>
add CONFIG proxy.config.diags.output.debug       STRING O
add CONFIG proxy.config.diags.output.diag        STRING O
add CONFIG proxy.config.diags.output.status      STRING O
add CONFIG proxy.config.diags.output.note        STRING O
add CONFIG proxy.config.diags.output.warning     STRING O
add CONFIG proxy.config.diags.output.error       STRING E
add CONFIG proxy.config.diags.output.fatal       STRING E 
add CONFIG proxy.config.diags.output.alert       STRING E
add CONFIG proxy.config.diags.output.emergency   STRING E

add CONFIG proxy.config.core_limit INT -1

[storage.config]

. 10000000
EOF

@ts_create_args = ("package", "ts",
 	           "localpath", "%%(ts_localpath)",
		   "config", $ts_config);

@sdktest_server_create_args = ("package", "sdktest_server",
			       "config", "");

# Plugin for SDKTest server must be specified on command line
#@sdktest_server_start_args = ("args", "-aServerVscan.so");
@sdktest_server_start_args = ();

# Caution !!!
# Substitution of %%(sdktest_server1) *not* permitted
# Only %%(sdktest_server1:server) works

# Note: custom parameters read by plugins can be added below
@sdktest_client_create_args = ("package", "sdktest_client", "config",
                               "proxy_host: %%(ts1)\n" .
                               "proxy_port: %%(ts1:tsHttpPort)\n" .
                               "server_host: %%(load1)\n" . 
                               "server_port: %%(sdktest_server1:server)\n" .
                               "users: 10\n" .
                               "hitrate: 60\n" .
                               "execution_interval: 30\n" .
                               "keepalive: 4\n" );

# Plugin for SDKTest client must be specified on command line
#@sdktest_client_start_args = ("args", "-pClientVscan.so");
@sdktest_client_start_args =();

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

# Start up SDKTest Server
TestExec::set_log_parser("sdktest_server1", "parse_sdktest");
TestExec::pm_create_instance("sdktest_server1", "%%(load1)", \@sdktest_server_create_args);
TestExec::pm_start_instance("sdktest_server1", \@sdktest_server_start_args);

# Wait for SDKTestServer port to become live
$r = TestExec::wait_for_server_port("sdktest_server1", "server", 10000);
if ($r < 0) {
    TestExec::add_to_log("Error: SDKTestServer failed to startup");
    die "SDKTestServer failed to start up\n";
}

# Start up SDKTest Client
TestExec::set_log_parser("sdktest_client1", "parse_sdktest");
TestExec::pm_create_instance("sdktest_client1", "%%(load2)", \@sdktest_client_create_args);
TestExec::pm_start_instance("sdktest_client1", \@sdktest_client_start_args);


# Note: the execution time of SDKtest_client is controlled by parameter
# execution_interval. So here we just wait until sdktest_client exits by itself
while (TestExec::is_instance_alive("sdktest_client1")) {
    TestExec::add_to_log("Client still alive");
    sleep(5);
}

# Clean up
TestExec::pm_stop_instance("sdktest_client1", \@empty_args);
TestExec::pm_destroy_instance("sdktest_client1", \@empty_args);

TestExec::pm_stop_instance("sdktest_server1", \@empty_args);
TestExec::pm_destroy_instance("sdktest_server1", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);

