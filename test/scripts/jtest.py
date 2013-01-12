#
#  jtest.py
#
#     A simple example script for using jtest under the DEFT
#      testing framework
#

ts_config = '''
[records.config]

proxy.config.diags.output.status      SE
proxy.config.diags.output.note 	      SE
proxy.config.diags.output.warning     SE
proxy.config.diags.output.error       SE
proxy.config.cache.storage_filename   storage.config
proxy.config.proxy_name		      deft.inktomi.com
proxy.config.core_limit		      -1
proxy.config.hostdb.size              50000
add CONFIG proxy.config.core_limit INT -1

[storage.config]


. 700000
'''

ts_create_args = ["package", "ts", "config", ts_config]
jtest_create_args = ["package", "jtest", "config", "-P %%(ts1) -p %%(ts1:tsHttpPort) -c 1"]
jtest_start_args = ["args", "-C"]
empty_args = []

# Check to see if we are using localpath ts
#ts_local = get_var_value("ts_localpath")
#if ts_local:
#    print "Using ts_localpath: $ts_local\n"
#    ts_create_args.append("localpath", ts_local)

# Start up the Traffic Server instance
print ts_create_args
pm_create_instance("ts1", "%%(ts1)", ts_create_args)
pm_start_instance("ts1", empty_args)

# Wait for the http port to becom live on TS
r = wait_for_server_port("ts1", "tsHttpPort", 60000)
if r < 0:
    add_to_log("Error: TS failed to startup")
    raise Exception("TS failed to start up\n")

# Start the jtest instance
print jtest_create_args

pm_create_instance("jtest1", "%%(load1)", jtest_create_args)
pm_start_instance("jtest1", jtest_start_args)

sleep(5)

raf_args1 = ["/stats/client_sec"]
raf_args2 = ["/processes/ts1/pid"]

# Run some queries a loop
i=0
while i < 20:
    raf_result = raf_instance("jtest1", "query", raf_args1)
    print "Raf Result: $raf_result[0] $raf_result[1] $raf_result[2]\n"

    raf_result = raf_proc_manager("ts1", "query", raf_args2)
    print "PM Raf Result: $raf_result[0] $raf_result[1] $raf_result[2]\n"
    sleep(2)
    i += 1

# Cleanup
pm_stop_instance("jtest1", empty_args)
pm_destroy_instance("jtest1", empty_args)

pm_stop_instance("ts1", empty_args)
pm_destroy_instance("ts1", empty_args)
