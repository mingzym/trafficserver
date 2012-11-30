
    SDK DEFT automated tests
    ------------------------


How to run the SDK DEFT tests ?
-------------------------------

o Build the plugins

 Go to directory traffic/proxy/api/samples and type in:
   #> ./configure
   #> gmake
   #> ./configure.internal
   #> gmake -f Makefile.internal


o Start your test

 Go to directory traffic/deft-install
 Start your DEFT test.

 Example 1: 
 Start test pool_functional.pl on a sun debug build, DEFT port starting at 20000
   #> run_test.sh sun_dbg -p 20000 -s plugins/thread-pool/pool_functional.pl -v

 Example 2:
 Start group of tests  SDK_full, sun optim build, DEFT port starting at 20000
   #> run_tests.sh sun_opt -p 20000 -g SDK_full -v


How to run a SDK DEFT test under Pure Coverage ?
------------------------------------------------

o Load up your pure environment and build TS.
   #> setenv PUREOPTIONS  "-cache-dir=`pwd`/pure_cache -best-effort -threads=yes
      -always-use-cache-dir=yes -max-threads=80 -free-queue-length=10000"

o Pass the '-k <sec>' flag to DEFT when running your tests,
  this will force it to wait longer when killing off a process.
  Purify writes it report files in the kill handler so this value
  has to be kicked way up from its default (2). sec = 300 is pretty good

 Example:
   #> run_test.sh COVERAGE -p 13000 -s plugins/add-header/add_functional.pl -v -k 300

o Once test is complete, PureCove generate a traffic_server.pcv files
  under the traffic server bin directory (ex.: traffic/sun_dbg/bin)

o To look at the coverage figures:
   #> purecov -view traffic_server.pcv



