#!/bin/bash
# SOURCE THIS FILE INTO YOUR ENVIRONMENT

USER=`basename $HOME`
DEFT_PORTS="${DEFT_PORTS:--p 12000}"

# export DEFT_PM_DEBUG='.*'

DEFT_ROOT=$HOME/ats_deft
export DEFT_ROOT

echo $PATH | grep ats_deft >/dev/null 2>&1 || export PATH=~/ats_deft/bin:$PATH

DEFT_HELP="

   DEFT Mode help:

   To run a program from the deft-install directory type the
   following:  

        test_exec <BUILD_DIR> [-m] [-v] -p <PORT> -s <SCRIPT> [-k val]

   The 'k' flag is used to lengthen the 'kill timeout' when running 
   instrumented binaries.  The 'v' flag starts up the event viewer,
   which is almost a must-have.
   
   To start the process manager separately (-m) do the 
   following steps:  First chdir to /inktest/<your directory>.  
   Then, run the process manager:  

        proc_manager* -p <PORT> -d . -T.\*

   If you do not want to use the GUI viewer [-v] then keep tail on
   the test log, it gets truncated with each new test; so you don't 
   need to re-tail the file.

        tail -f $DEFT_ROOT/log/test.log

   Examples:

   test_exec BUILD $DEFT_PORTS -s plugins/thread-1/thread_functional.pl -v
   test_exec BUILD $DEFT_PORTS -g SDK_full
   test_exec COVERAGE $DEFT_PORTS -v my_test.pl -k 300

   See scripts/acc/http/ldap/ldap-1.pl for a example syntest driven script.

   DEFT helper commands:

    cd_deft() changes to the deft install directory
    deft_check <script.pl>  will do a perl syntax check of a perl script.

      Example: cd_deft; cd scripts/plugins/null-transform; deft_check null_functional.pl

   debugging:
    deft_start_proc() starts the process manager with debugging tags turned on.
    deft_start_test() runs a test in debug mode against the manual process mgr.
"

deft_start_proc() {
  local here=`pwd`
  cd $DEFT_ROOT
  proc_manager $DEFT_PORTS -d . -T.\*;
  cd $here
}

deft_start_test() {
   local build=$1;
   local deft_script=$2;
   local here=`pwd`
   cd $DEFT_ROOT
   test_exec $build -m $DEFT_PORTS -s $deft_script -T.\*
   cd $here
}

cd_deft() {
   cd $DEFT_ROOT
}

deft_check () {
   if [ -f $1 ]
   then
	mkdir -p $DEFT_ROOT/tmp/
	local NF=$DEFT_ROOT/tmp/$(basename $1)
	echo 'sub TestExecc::boot_TestExec() {}' > $NF
        cat $1 >> $NF
	perl -cw $NF
   else
        echo "where is the $1 ???"
   fi
}

echo "$DEFT_HELP";
