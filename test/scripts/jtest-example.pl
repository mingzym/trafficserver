#
#  jtest-example.pl
#
#     A simple example script for using jtest under the DEFT
#      testing framework
#
#  $Id: jtest-example.pl,v 1.2 2003-06-01 18:38:30 re1 Exp $
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
                   "config", $ts_config);

@jtest_create_args = ("package", "jtest",
                      "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 5");

@jtest_start_args = ("args", "-C");
@empty_args = ();

# Check to see if we are using localpath ts
$ts_local = TestExec::get_var_value("ts_localpath");
if ($ts_local) {
    print "Using ts_localpath: $ts_local\n";
    push(@ts_create_args, "localpath", $ts_local);
}

# Start up the Traffic Server instance
TestExec::pm_create_instance("ts1", "%%(ts1)", \@ts_create_args);
TestExec::pm_start_instance("ts1", \@empty_args);

# Wait for the http port to becom live on TS
$r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
if ($r < 0) {
    TestExec::add_to_log("Error: TS failed to startup");
    die "TS failed to start up\n";
}

# Start the jtest instance
TestExec::pm_create_instance("jtest1", "%%(load1)", \@jtest_create_args);
TestExec::pm_start_instance("jtest1", \@jtest_start_args);

# Wait a few seconds for jtest to get underway
sleep(10);

@raf_args1 = "/stats/client_sec";
@raf_result = TestExec::raf_instance("jtest1", "query", \@raf_args1);

# Must do at five ops per second
if ($raf_result[2] < 5) {
   TestExec::add_to_log("Error: insufficent jtest ops: $raf_result[2]"); 
} else {
   TestExec::add_to_log("Status: jtest ops: $raf_result[2]"); 
}

TestExec::pm_stop_instance("jtest1", \@empty_args);
TestExec::pm_destroy_instance("jtest1", \@empty_args);

TestExec::pm_stop_instance("ts1", \@empty_args);
TestExec::pm_destroy_instance("ts1", \@empty_args);





