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

add CONFIG proxy.config.nntp.enabled INT 1
add CONFIG proxy.config.nntp.server_port INT %%(ts1_extra_ports:tsNntpPort)
add CONFIG proxy.config.nntp.cache_enabled INT 1
add CONFIG proxy.config.nntp.posting_enabled INT 1
add CONFIG proxy.config.nntp.logging_enabled INT 0
add CONFIG proxy.config.nntp.transparency_enabled INT 0
add CONFIG proxy.config.nntp.client_timeout INT 300
add CONFIG proxy.config.nntp.server_timeout INT 300
add CONFIG proxy.config.nntp.expire_group INT 600
add CONFIG proxy.config.nntp.expire_overview INT 600
add CONFIG proxy.config.nntp.expire_list_active INT 86400
add CONFIG proxy.config.nntp.expire_list_atimes INT 86400

add CONFIG proxy.config.nntp.greeting STRING NNTP service ready
add CONFIG proxy.config.nntp.greeting_nopost STRING NNTP service ready; posting prohibited

add CONFIG proxy.config.nntp.show_enabled INT 0
add CONFIG proxy.config.nntp.debug_enabled INT 0
add CONFIG proxy.config.nntp.overview_range_size INT 512

[storage.config]

. 64000000

[plugins.config]
nntp.so

EOC

@ts_extra_ports_create_args = ("package", "allocate_ports",
                               "config", "tsNntpPort");

@ts_create_args = ("package", "ts",
                   "localpath", "%%(ts_localpath)",
                   "config", $ts_config);

@ts_start_args = ("args", "-k -K");


@loadgen_nntp_server_create_args = ("package", "loadgen_nntp",
                      "config", "-m 2 -S %%(load1)");


@loadgen_nntp_client_create_args = ("package", "loadgen_nntp",
                      "config", "-m 1 -P %%(ts1) -p %%(ts1_extra_ports:tsNntpPort) -S %%(load2) -c 10");

@loadgen_nntp_server_start_args = ("args", "-t testplans/testcase.xml");

@loadgen_nntp_client_start_args = ("args", "-t testplans/testcase.xml");


@empty_args = ();

# Start up the loadgen_nntp server instance
# (we need the listening port number to configure traffic server)
TestExec::set_log_parser("loadgen_nntp_server", "parse_loadgen");
TestExec::pm_create_instance("loadgen_nntp_server", "%%(load1)", \@loadgen_nntp_server_create_args);
TestExec::pm_start_instance("loadgen_nntp_server", \@loadgen_nntp_server_start_args);
TestExec::add_to_log("loadgen_nntp server started successfully");
$r = TestExec::wait_for_server_port("loadgen_nntp_server", "server", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: loadgen_nntp server failed to startup");
    die "loadgen_nntp server failed to start up\n";
}


# Allocate one port for Traffic Server nntp port
TestExec::pm_create_instance("ts1_extra_ports", "%%(ts1)", \@ts_extra_ports_create_args);

# Start up the Traffic Server instance
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::put_instance_file_subs("ts1", "config/nntp_config.xml", "nntp_config.xml");
TestExec::pm_start_instance("ts1", \@ts_start_args);

# Wait for the nntp port to become live on TS
$r = TestExec::wait_for_server_port("ts1_extra_ports", "tsNntpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS NNTP failed to startup");
    die "TS NNTP failed to start up\n";
}

# Start the loadgen instances
TestExec::set_log_parser("loadgen_nntp_client", "parse_loadgen");
TestExec::pm_create_instance("loadgen_nntp_client", "%%(load2)", \@loadgen_nntp_client_create_args);
TestExec::pm_start_instance("loadgen_nntp_client", \@loadgen_nntp_client_start_args);
TestExec::add_to_log("loadgen_nntp_client started successfully");


# Keep test running for 60sec
sleep(60);


# Query loadgen via raf interface to get stats
@raf_args1 = "/stat/client/ops_total";
@raf_result1 = TestExec::raf_instance("loadgen_nntp_server", "query", \@raf_args1);

@raf_args2 = "/stat/server/ops_total";
@raf_result2 = TestExec::raf_instance("loadgen_nntp_client", "query", \@raf_args1);

# Must have done at least 50 operations on client and server
if (($raf_result1[2] < 50) or ($raf_result2[2] < 50)) {
   TestExec::add_to_log("Error: insufficent loadgen ops - client: " . $raf_result1[2] . " server: " . $raf_result2[2]);
} else {
   TestExec::add_to_log("Status: loadgen ops - client: " . $raf_result1[2] . " server: " . $raf_result2[2]);
}


# Stop the load and TS
TestExec::pm_stop_instance("loadgen_nntp_client", \@empty_args);
TestExec::pm_destroy_instance("loadgen_nntp_client", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);

TestExec::pm_stop_instance("loadgen_nntp_server", \@empty_args);
TestExec::pm_destroy_instance("loadgen_nntp_server", \@empty_args);


