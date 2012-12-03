#
#  jtest-1server-1client-example.pl
#  author: stephane (from jtest-example.pl)
#  creation: 03/27/02
#
#     A simple example script for using jtest under the DEFT
#     testing framework with 1 jtest server and 1 jtest client.
#     The jtest client is configured with the port allocated
#     for the jtest server.
#
#  $Id: jtest-1server-1client-example.pl,v 1.2 2003-06-01 18:38:30 re1 Exp $
#

use TestExec;

$ts_config = <<EOC;
[records.config]

proxy.config.diags.output.status      SE
proxy.config.diags.output.note 	      SE
proxy.config.diags.output.warning     SE
proxy.config.diags.output.error       SE
proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name               deft.inktomi.com
proxy.config.hostdb.size              50000
add CONFIG proxy.config.core_limit INT -1

[storage.config]


. 700000
EOC

@ts_create_args = ("package", "ts",
                   "localpath", "%%(ts_localpath)",
                   "config", $ts_config);

# Note: the -y/-Y must be in the create_args because the
# jtest instantiator use them to determine if it has to
# allocate a server port.

@jtest_server_create_args = ("package", "jtest",
                      "config", "-Y");

@jtest_client_create_args = ("package", "jtest",
                      "config", "-y");

@jtest_server_start_args = ("args", "-C");
@jtest_client_start_args = ("args", "-C -S %%(load1) -s %%(jtest_server:server) -P %%(ts1) -p %%(ts1:tsHttpPort) -c 5");

@empty_args = ();

# Start up the Traffic Server instance
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for the http port to become live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the jtest server instance
TestExec::pm_create_instance("jtest_server", "%%(load1)", \@jtest_server_create_args);
TestExec::pm_start_instance("jtest_server", \@jtest_server_start_args);

# Start the jtest client instance
TestExec::pm_create_instance("jtest_client", "%%(load2)", \@jtest_client_create_args);
TestExec::pm_start_instance("jtest_client", \@jtest_client_start_args);

# Wait a few seconds for jtest to get underway
sleep(10);

@raf_args_jtest_client = "/stats/client_sec";
@raf_result_jtest_client = TestExec::raf_instance("jtest_client", "query", \@raf_args_jtest_client);

# Must do at five ops per second
if ($raf_result_jtest_client[2] < 5) {
   TestExec::add_to_log("Error: insufficent client jtest ops: $raf_result_jtest_client[2]"); 
} else {
   TestExec::add_to_log("Status: client jtest ops: $raf_result_jtest_client[2]"); 
}


TestExec::pm_stop_instance("jtest_client", \@empty_args);
TestExec::pm_destroy_instance("jtest_client", \@empty_args);

TestExec::pm_stop_instance("jtest_server", \@empty_args);
TestExec::pm_destroy_instance("jtest_server", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);





