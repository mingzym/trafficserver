#
#  conversion.pl
#
#    tests for conversion of filter.config to policy_config.xml
#
#  $Author: re1 $
#
#  $Id: conversion.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $
#
#  Notes:
#    Try to make sure that there is only one "ERROR" log statement
#    for each failed test.
#
#  Test 01 uses a filter.config containing only the snmp blocking rule
#  Test 02 uses the big parseing tester that needs to be periodically updated
#          from traffic/auth/config to this file:
#                traffic/test/scripts/acc/http/policy/filter.config_test2 
#  Test 03 filter_to_policy hang
#  Test 04 no policy_config.xml (f2p exits w/ err(1))
#  Test 05 empty policy_config.xml
#  Test 06 empty filter.config
#  Test 07 missing filter.config
#
#  Remember that this script will need to be updated often - or made smarter
#  to avoid those updates.  For example, when the conversion script is
#  updated, it will naturally have a new cvs version.  Since this cvs version
#  is printed out at the top of the policy_config.xml that it creates, and
#  because we do a simple diff of the new file with the "gold" file, every new
#  update in the conversion script will require an update to the gold files.
#  
#  Also, if we update the test filter.config we will also have to update the
#  local copy of this file found in this directory as filter.config_test2 and
#  the gold file for the conversion of this file, gold_02.
# 
#  EG, cd mainline/traffic
#      cp auth/config/filter.config test/scripts/acc/http/policy/filter.config_test2
#      [ and then run the test and copy the new policy_config.xml to the gold 
#        file. ]
#      <comment script so only test 2 runs>
#      ./run_test.pl sun_dbg -p 100X -s acc/http/policy/conversion.pl -v
#      cp /inktest /inktest/davidl-0/run/ts1/config/policy_config.xml \
#                  <there>/traffic/test/scripts/acc/http/policy/gold_02


#------------------------------------------------------------------------------#
#                            EXTERNAL RESOURCES                                #
use Cwd;
use TestExec;
use ConfigHelper;
use PolicyConfig;
use strict;
#------------------------------------------------------------------------------#


#------------------------------------------------------------------------------#
#                                   VERSION                                    #
my $CVS_ID_TAG   = '$Id: conversion.pl,v 1.2 2003-06-01 18:38:31 re1 Exp $';
my $CVS_VER_TAG  = '$Revision: 1.2 $';
my $CVS_SOURCE   = '$Source: /home/bcall/cvs_to_svn/CVSROOT/yahoo/properties/ycdn/traffic/test/scripts/acc/http/policy/conversion.pl,v $ ';
my ($CVS_VER)    = $CVS_VER_TAG =~ /([0-9\.]+)/;
my ($CVS_FILE)   = $CVS_ID_TAG =~ /Id: (.*),v /;
my ($SCRIPT_DIR) = $CVS_SOURCE =~ /\/CVS_REPOSITORY\/traffic\/test\/(.*)\/$CVS_FILE,v/;
#------------------------------------------------------------------------------#


#------------------------------------------------------------------------------#
#                           CONFIGURATION CONSTANTS                            #
my $UPDATE_TARBALL = 0;
my $SHOW_HEADER = 1;
my $QUIET = 0;
my $VERBOSE = 1;
#------------------------------------------------------------------------------#



#------------------------------------------------------------------------------#
#                              OTHER GLOBALS                                   #
my %fstat;
my $raf_manager_port;
#------------------------------------------------------------------------------#


# and do it!
&main();

################################################################################
################################################################################
#                                                                              #
#                                 MAIN                                         #
#                                                                              #
################################################################################
sub main {

    &show_version() unless ($QUIET > 0);
    
    &run_test_1();
    &run_test_2();
    &run_test_3();
    &run_test_4();
    &run_test_5();
    &run_test_6();
    &run_test_7();
    
    &show_end_message() unless ($QUIET > 0);
  exit 0;
}

################################################################################
sub show_version {
    my $msg = "$CVS_FILE version $CVS_VER";
    LOG("=" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    $msg = "\$SCRIPT_DIR = \"$SCRIPT_DIR\"";
    VERBOSE(" " x ((60 - length($msg)) / 2) . $msg);
}


################################################################################
sub show_end_message {
    my $msg = "Tests Finished";
    TestExec::add_to_log("-" x 60);
    TestExec::add_to_log(" " x ((60 - length($msg)) / 2) . $msg);
    TestExec::add_to_log("=" x 60);
}

################################################################################
sub run_test_1 {
    my($msg,$gold,$fyl,$failure);
    $gold = getcwd() . "/$SCRIPT_DIR/gold_01";

    $msg = "Starting Test 01 - correctness test on default filter.config file";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # copy over default filter.config
    TestExec::put_instance_file_raw('ts1', 'config/filter.config',
				    'filter.config_test1');
    
    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    VERBOSE("-" x 60);
    VERBOSE("Sleeping 17 seconds to let TS come completely up");
    VERBOSE("-" x 60);
    sleep 17;

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (exists $fstat{'size'}) {
	$fyl = TestExec::get_instance_file('ts1','config/policy_config.xml');
	VERBOSE("-" x 60);
	VERBOSE("policy_config.xml is $fstat{size} bytes");
	VERBOSE("Local copy of policy_config.xml is $fyl");
	VERBOSE("-" x 60);
    } else {
        LOG("-" x 60);
        LOG("ERROR: Couldn't retrieve policy_config.xml");
        LOG("-" x 60);
	goto END_OF_TEST;
    }    

    # Do file "diff" - make sure that the created policy_config.xml is good
    $failure = &compare_files($gold,$fyl);
    if ($failure) {
	my $msg = "Test 01 FAILED";
	LOG("ERROR: Test 01 FAILED - policy_config.xml comparison failed");
	NOT_QUIET();
        NOT_QUIET("-" x 60);
	NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
	NOT_QUIET("-" x 60);
    } else {
	my $msg = "Test 01 PASSED";
        NOT_QUIET("-" x 60);
        NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
        NOT_QUIET("-" x 60);
    }

  END_OF_TEST:
    TestExec::pm_stop_instance("ts1", \@empty_args);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub run_test_2 {
    my($msg,$gold,$fyl,$failure);
    $gold = getcwd() . "/$SCRIPT_DIR/gold_02";

    $msg = "Starting Test 02 - correctness test on test filter.config file";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # copy over default filter.config
    TestExec::put_instance_file_raw('ts1', 'config/filter.config',
				    'filter.config_test2');
    
    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    VERBOSE("-" x 60);
    VERBOSE("Sleeping 17 seconds to let TS come completely up");
    VERBOSE("-" x 60);
    sleep 17;

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (exists $fstat{'size'}) {
	$fyl = TestExec::get_instance_file('ts1','config/policy_config.xml');
	VERBOSE("-" x 60);
	VERBOSE("policy_config.xml is $fstat{size} bytes");
	VERBOSE("Local copy of policy_config.xml is $fyl");
	VERBOSE("-" x 60);
    } else {
        LOG("-" x 60);
        LOG("ERROR: Couldn't retrieve policy_config.xml");
        LOG("-" x 60);
	goto END_OF_TEST;
    }    

    # Do file "diff" - make sure that the created policy_config.xml is good
    $failure = &compare_files($gold,$fyl);
    if ($failure) {
	my $msg = "Test 02 FAILED";
	LOG("ERROR: Test 02 FAILED - policy_config.xml comparison failed");
	NOT_QUIET();
        NOT_QUIET("-" x 60);
	NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
	NOT_QUIET("-" x 60);
    } else {
	my $msg = "Test 02 PASSED";
        NOT_QUIET("-" x 60);
        NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
        NOT_QUIET("-" x 60);
    }

  END_OF_TEST:
    TestExec::pm_stop_instance("ts1", \@empty_args);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub run_test_3 {
    my($msg,$fyl,$failure,$fake_bin,$orig,$slp_t);
    $fake_bin = getcwd() . "/$SCRIPT_DIR/hang";

    $msg = "Starting Test 03 - filter_to_policy hangs - no policy_config.xml";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # create hanging filter_to_policy by copying in a bourne shell
    # script that hangs for an hour.
    open(FAK,">$fake_bin") or die "Can't write tmp file $fake_bin";
    print FAK '#!/bin/sh' . "\n";
    print FAK "rm -f config/policy_config.xml\n";
    print FAK "while : ; do sleep 1 ; done\n";
    print FAK "\n";
    close(FAK);
    chmod(0755,$fake_bin);

    # get copy of orginal conversion program
    $orig = TestExec::get_instance_file('ts1','bin/filter_to_policy');
    LOG("Orig conv: $orig");
    LOG("New  conv: $fake_bin");

    # copy our broken script over the good one - note that this will
    # wipe out your installation's copy of the script - that's why we
    # get ourselves a copy above - so we can put the actual one back
    # when we are done
    TestExec::put_instance_file_raw('ts1', 'bin/filter_to_policy',
				    $fake_bin);
    unlink($fake_bin);

    sleep(3600);

    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    $slp_t = 90;
    VERBOSE("-" x 60);
    VERBOSE("Sleeping $slp_t seconds to let TS come completely up");
    VERBOSE("-" x 60);
    $slp_t += time();
    while (time() < $slp_t) { sleep(5) }

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (exists $fstat{'size'}) {
        LOG("-" x 60);
        LOG("WARNING: policy_config.xml exists - it shouldn't - "
	    . "something wrong with the script?");
        LOG("-" x 60);
    }    

  END_OF_TEST:
    TestExec::put_instance_file_raw('ts1','bin/filter_to_policy',$orig);
    TestExec::pm_stop_instance("ts1", \@empty_args);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub run_test_4 {
    my($msg,$fyl,$failure,$fake_bin,$orig,$slp_t);
    $fake_bin = getcwd() . "/$SCRIPT_DIR/hang";

    $msg = "Starting Test 04 - no policy_config.xml (f2p exits w/ err(1))";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # create hanging filter_to_policy by copying in a bourne shell
    # script that hangs for an hour.
    open(FAK,">$fake_bin") or die "Can't write tmp file $fake_bin";
    print FAK '#!/bin/sh' . "\n";
    print FAK "rm -f config/policy_config.xml\n";
    print FAK "exit 1\n";
    print FAK "\n";
    close(FAK);
    chmod(0755,$fake_bin);

    # get copy of orginal conversion program
    $orig = TestExec::get_instance_file('ts1','bin/filter_to_policy');
    LOG("Orig conv: $orig");
    LOG("New  conv: $fake_bin");

    # copy our broken script over the good one - note that this will
    # wipe out your installation's copy of the script - that's why we
    # get ourselves a copy above - so we can put the actual one back
    # when we are done
    TestExec::put_instance_file_raw('ts1', 'bin/filter_to_policy',
				    $fake_bin);
    unlink($fake_bin);

    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    $slp_t = 90;
    VERBOSE("-" x 60);
    VERBOSE("Sleeping $slp_t seconds to let TS come completely up");
    VERBOSE("-" x 60);
    $slp_t += time();
    while (time() < $slp_t) { sleep(5) }

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (exists $fstat{'size'}) {
        LOG("-" x 60);
        LOG("WARNING: policy_config.xml exists - it shouldn't - "
	    . "something wrong with the script?");
        LOG("-" x 60);
    }    

  END_OF_TEST:
    TestExec::put_instance_file_raw('ts1','bin/filter_to_policy',$orig);
    TestExec::pm_stop_instance("ts1", \@empty_args);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub run_test_5 {
    my($msg,$fyl,$failure,$fake_bin,$orig,$slp_t);
    $fake_bin = getcwd() . "/$SCRIPT_DIR/hang";

    $msg = "Starting Test 05 - empty policy_config.xml";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # create hanging filter_to_policy by copying in a bourne shell
    # script that hangs for an hour.
    open(FAK,">$fake_bin") or die "Can't write tmp file $fake_bin";
    print FAK '#!/bin/sh' . "\n";
    print FAK "rm -f config/policy_config.xml\n";
    print FAK "touch config/policy_config.xml\n";
    print FAK "exit 0\n";
    print FAK "\n";
    close(FAK);
    chmod(0755,$fake_bin);

    # get copy of orginal conversion program
    $orig = TestExec::get_instance_file('ts1','bin/filter_to_policy');
    LOG("Orig conv: $orig");
    LOG("New  conv: $fake_bin");

    # copy our broken script over the good one - note that this will
    # wipe out your installation's copy of the script - that's why we
    # get ourselves a copy above - so we can put the actual one back
    # when we are done
    TestExec::put_instance_file_raw('ts1', 'bin/filter_to_policy',
				    $fake_bin);
    unlink($fake_bin);

    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    $slp_t = 90;
    VERBOSE("-" x 60);
    VERBOSE("Sleeping $slp_t seconds to let TS come completely up");
    VERBOSE("-" x 60);
    $slp_t += time();
    while (time() < $slp_t) { sleep(5) }

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (!exists $fstat{'size'}) {
        LOG("-" x 60);
        LOG("ERROR: Couldn't retrieve policy_config.xml");
        LOG("-" x 60);
    } elsif ($fstat{'size'} != 0) {
        LOG("-" x 60);
        LOG("WARNING: on this test policy_config.xml should be zero "
	    . "bytes - it's $fstat{'size'} bytes instead");
        LOG("-" x 60);
    }

  END_OF_TEST:
    TestExec::put_instance_file_raw('ts1','bin/filter_to_policy',$orig);
    TestExec::pm_stop_instance("ts1", \@empty_args);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub run_test_6 {
    my($msg,$gold,$fyl,$failure,$helper_bin,$orig,$slp_t,$result);
    $gold = getcwd() . "/$SCRIPT_DIR/gold_06";
    $helper_bin = getcwd() . "/$SCRIPT_DIR/helper.sh";

    $msg = "Starting Test 06 - empty filter.config";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # get copy of orginal filter.config
    #$orig = TestExec::get_instance_file('ts1','config/filter.config');
    #NOT_QUIET("Orig filter.config: $orig");

    # create script to zero out filter.config
    open(FAK,">$helper_bin") or die "Can't write tmp file $helper_bin";
    print FAK '#!/bin/sh' . "\n";
    print FAK "cp /dev/null config/filter.config\n";
    print FAK "exit 0\n";
    print FAK "\n";
    close(FAK);
    chmod(0755,$helper_bin);

    TestExec::put_instance_file_raw('ts1', 'bin/helper.sh', $helper_bin);
    unlink($helper_bin);
    $result = TestExec::pm_run_slave('ts1',"bin/helper.sh","",10000);
    if ($result != 0) {
        LOG("Error: Failed to run script to zero out filter.config");
	goto END_OF_TEST;
    }
    
    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    $slp_t = 60;
    VERBOSE("-" x 60);
    VERBOSE("Sleeping $slp_t seconds to let TS come up and go quiet");
    VERBOSE("-" x 60);
    $slp_t += time();
    while (time() < $slp_t) { sleep(5) }

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (exists $fstat{'size'}) {
	$fyl = TestExec::get_instance_file('ts1','config/policy_config.xml');
	VERBOSE("-" x 60);
	VERBOSE("policy_config.xml is $fstat{size} bytes");
	VERBOSE("Local copy of policy_config.xml is $fyl");
	VERBOSE("-" x 60);
    } else {
        LOG("-" x 60);
        LOG("ERROR: Couldn't retrieve policy_config.xml");
        LOG("-" x 60);
	goto END_OF_TEST;
    }    

    # Do file "diff" - make sure that the created policy_config.xml is good
    $failure = &compare_files($gold,$fyl);
    if ($failure) {
	my $msg = "Test 06 FAILED";
	LOG("ERROR: Test 06 FAILED - policy_config.xml comparison failed");
	NOT_QUIET();
        NOT_QUIET("-" x 60);
	NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
	NOT_QUIET("-" x 60);
    } else {
	my $msg = "Test 06 PASSED";
        NOT_QUIET("-" x 60);
        NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
        NOT_QUIET("-" x 60);
    }

  END_OF_TEST:
    TestExec::pm_stop_instance("ts1", \@empty_args);
    #TestExec::put_instance_file_raw('ts1','config/filter.config',$orig);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub run_test_7 {
    my($msg,$gold,$fyl,$failure,$helper_bin,$orig,$slp_t,$result);
    $gold = getcwd() . "/$SCRIPT_DIR/gold_07";
    $helper_bin = getcwd() . "/$SCRIPT_DIR/helper.sh";

    $msg = "Starting Test 07 - missing filter.config";
    LOG("-" x 60);
    LOG(" " x ((60 - length($msg)) / 2) . $msg);
    LOG("-" x 60);

    # TS configuration
    my $cfg = new ConfigHelper('SkipPolicyGeneration' => '1');
    # $cfg->set_debug_tags('acc.*|http_hdrs|http_auth|policy.*');
    $cfg->set_record('proxy.config.proxy_binary_opts','-M -jKk');

    # because manager defaults to 20098 - and I'm not the only one
    # running ts so I don't always get to use this port
    $raf_manager_port = TestExec::pm_alloc_port('%%(ts1)');
    if ($raf_manager_port < 0) {
        LOG("Error: couldn't find free port"); die;
    }
    $cfg->add_record('INT','proxy.config.raf.manager.port',$raf_manager_port);
    $cfg->add_config('meta');
    $cfg->add_config_line('run_manager: 1');
    
    my @ts_start_args = ();
    my @empty_args = ();
    my @ts_create_args = ("package", "ts", 
			   "localpath","%%(ts_localpath)",
			   "config",$cfg->output);
    
    # Traffic Server config + startup
    TestExec::pm_create_instance('ts1', "%%(ts1)", \@ts_create_args);

    &show_header() unless ($SHOW_HEADER < 1);

    # get copy of orginal filter.config
    #$orig = TestExec::get_instance_file('ts1','config/filter.config');
    #NOT_QUIET("Orig filter.config: $orig");

    # create script to zero out filter.config
    open(FAK,">$helper_bin") or die "Can't write tmp file $helper_bin";
    print FAK '#!/bin/sh' . "\n";
    print FAK "rm config/filter.config\n";
    print FAK "exit 0\n";
    print FAK "\n";
    close(FAK);
    chmod(0755,$helper_bin);

    TestExec::put_instance_file_raw('ts1', 'bin/helper.sh', $helper_bin);
    unlink($helper_bin);
    $result = TestExec::pm_run_slave('ts1',"bin/helper.sh","",10000);
    if ($result != 0) {
        LOG("Error: Failed to run script to zero out filter.config");
	goto END_OF_TEST;
    }
    
    # Start TS
    TestExec::pm_start_instance("ts1", \@ts_start_args);

    # Wait for TS http port to become live
    # this doesn't work very well when you start traffic manager instead
    # of just traffic_server - don't know why
    my $r = TestExec::wait_for_server_port("ts1", "tsHttpPort", 60000);
    if ($r < 0) {
	LOG("Error: TS failed to startup");
	die "TS failed to start up\n";
    }

    $slp_t = 60;
    VERBOSE("-" x 60);
    VERBOSE("Sleeping $slp_t seconds to let TS come up and go quiet");
    VERBOSE("-" x 60);
    $slp_t += time();
    while (time() < $slp_t) { sleep(5) }

    %fstat = TestExec::stat_instance_file('ts1','config/policy_config.xml');
    if (exists $fstat{'size'}) {
	$fyl = TestExec::get_instance_file('ts1','config/policy_config.xml');
	VERBOSE("-" x 60);
	VERBOSE("policy_config.xml is $fstat{size} bytes");
	VERBOSE("Local copy of policy_config.xml is $fyl");
	VERBOSE("-" x 60);
    } else {
        LOG("-" x 60);
        LOG("ERROR: Couldn't retrieve policy_config.xml");
        LOG("-" x 60);
	goto END_OF_TEST;
    }    

    # Do file "diff" - make sure that the created policy_config.xml is good
    $failure = &compare_files($gold,$fyl);
    if ($failure) {
	my $msg = "Test 07 FAILED";
	LOG("ERROR: Test 07 FAILED - policy_config.xml comparison failed");
	NOT_QUIET();
        NOT_QUIET("-" x 60);
	NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
	NOT_QUIET("-" x 60);
    } else {
	my $msg = "Test 07 PASSED";
        NOT_QUIET("-" x 60);
        NOT_QUIET(" " x ((60 - length($msg)) / 2) . $msg);
        NOT_QUIET("-" x 60);
    }

  END_OF_TEST:
    TestExec::pm_stop_instance("ts1", \@empty_args);
    #TestExec::put_instance_file_raw('ts1','config/filter.config',$orig);
    TestExec::pm_destroy_instance("ts1", \@empty_args);

    return;

}


################################################################################
sub compare_files {
    my $gold = shift;
    my $fyl = shift;

    my(@lines,$cmd,$f_sum,$f_siz,$g_sum,$g_siz);
    
    # return zero(0) on success and one(1) otherwise.

    if (!files_readable($gold,$fyl)) {
	my $msg = "WARNING: Can't do policy_config.xml comparison because we ";
	$msg .= "either can't find the new file or we can't find the gold file";
        LOG($msg);
	return 1;
    }

    $cmd = "cksum $gold 2>&1|";
    open(CMD,$cmd) or die "Can't execute cksum";
    @lines = <CMD>;
    close(CMD);

    VERBOSE("Got " . ($#lines + 1) . " line(s) from cksum command");
    VERBOSE(@lines);

    if ($lines[0] =~ /^(\d+)\s+(\d+)\s+/) {
	$g_sum = $1;
	$g_siz = $2;
	VERBOSE("Gold file CRC: $g_sum    SIZ: $g_siz");
    }

    $cmd = "cksum $fyl 2>&1|";
    open(CMD,$cmd) or die "Can't execute cksum";
    @lines = <CMD>;
    close(CMD);

    VERBOSE("Got " . ($#lines + 1) . " line(s) from cksum command");
    VERBOSE(@lines);

    if ($lines[0] =~ /^(\d+)\s+(\d+)\s+/) {
	$f_sum = $1;
	$f_siz = $2;
	VERBOSE("Test file CRC: $f_sum    SIZ: $f_siz");
    }

    if ($f_sum != $g_sum || $f_siz != $g_siz) {
	my $msg  = "WARNING: policy_config.xml comparison failed -  ";
	$msg    .= "checksums don't match";
        LOG($msg);
	return 1;
    }

    return 0;
}


################################################################################
sub files_readable {
    my @files = @_;
    my $fyl;
    my $err = 1;

    foreach $fyl (@files) {
	if (-f $fyl) {
	      if (-r $fyl) {
		  TestExec::add_to_log("NOTE: file found and readable: \"$fyl\"") 
		      unless ($VERBOSE < 1);
	      } else {
		  TestExec::add_to_log("WARNING: file found but unreadable: \"$fyl\"") ;
		  $err = 0;
	      }
	} else {
	    my $msg = "WARNING: Couldn't find file: \"$fyl\" in ";
	    $msg .= '"' . getcwd() . '"';
	    TestExec::add_to_log($msg);
	    $err = 0;
	}
	
    }
    return $err;
}


################################################################################
sub show_header {
    my $proxy_port = TestExec::get_var_value('ts1:tsHttpPort');
    my $proxy_server = TestExec::get_var_value('ts1');
    my $raf_port = TestExec::get_var_value('ts1:rafPort');
    
    TestExec::add_to_log("-" x 60);
    TestExec::add_to_log("Traffic Server host: $proxy_server");
    TestExec::add_to_log("HTTP Proxy port:     $proxy_port");
    TestExec::add_to_log("RAF service port:    $raf_port");
    TestExec::add_to_log("RAF manager port:    $raf_manager_port");
    TestExec::add_to_log("-" x 60);
    #TestExec::add_to_log("Policy Config:");
    #foreach my $ln ((split /\n/, $pcfg_text)) {
    #   TestExec::add_to_log("  $ln");
    #}
}


################################################################################
sub LOG {
    my $line;
    foreach $line (@_) {
      TestExec::add_to_log($line);
    }
}

################################################################################
sub NOT_QUIET {
    my $line;
    if ($QUIET < 1) {
	foreach $line (@_) {
          TestExec::add_to_log($line);
	}
    }
}

################################################################################
sub VERBOSE {
    my $line;
    if ($VERBOSE > 0) {
	foreach $line (@_) {
          TestExec::add_to_log($line);
	}
    }
}

################################################################################
sub old_syntest {
    # probably not used

    if ($UPDATE_TARBALL) {
	chomp(my $run_path = `dirname $0`);
	chomp(my $cur_path = `pwd`);
	# my $cur_path = $ENV{PWD};
	
        TestExec::add_to_log("Note: run_path = $run_path");
        TestExec::add_to_log("Note: cur_path = $cur_path");
	chdir($run_path) or die "chdir failed";
	unlink('conversion.tar');
	system('tar cf conversion.tar Tests');
	chdir($cur_path) or die "chdir failed";
    }
    
    my @syntest_create_args = 
	( "package", "syntest", "config", 
	 "proxy_host: %%(ts1)\nproxy_port: %%(ts1:tsHttpPort)\n");
    my @syntest_start_args = 
	("args", "-f conversion.cfg -c Policy-TimeRanges -noquit");

    # Syntest config + startup
    TestExec::pm_create_instance ("syntest1", "%%(load1)", 
				  \@syntest_create_args);


    # Install
    TestExec::put_instance_file_raw("syntest1", "conversion.cfg",   
				    "conversion.cfg");
    TestExec::put_instance_file_raw("syntest1", "conversion.tar", 
				    "conversion.tar");
    TestExec::put_instance_file_raw("syntest1", "untar.sh", 
				    "../../../plugins/common/untar.sh");
    my $result = TestExec::pm_run_slave("syntest1",
					 "untar.sh",
					 "conversion.tar",
					 10000);
    if ($result != 0) {
        TestExec::add_to_log("Error: Failed to install test cases");
	die;
    }
    
    # Now start syntest
    TestExec::pm_start_instance("syntest1", \@syntest_start_args);
    
    # Test execution
    sleep(5);
    
    my @raf_args1 = "/processes/syntest1/pid";
    
    sleep(2);
    TestExec::add_to_log("Waiting for syntest1 to exit\n");
    print "Waiting for syntest1 to exit\n";
    
    my $MAX_SYNTEST_RUN = 30;
    my $start_time = time;
    
    # Loop waiting for syntest to finish
    while (1) {
	my $curr_time = time;
	
	if (($curr_time - $start_time) > $MAX_SYNTEST_RUN) {
	    TestExec::add_to_log("Error: aborting syntest1 instance, timeout");
	    last;
	}
	
	my @raf_result = TestExec::raf_proc_manager("syntest1", "query", 
						    \@raf_args1);
	if (scalar(@raf_result != 3) || $raf_result[0] != 0 || 
	    $raf_result[2] < 0) 
	{
	    last;
	} else {
	    sleep(15);
	}
    }

    #TestExec::pm_stop_instance("syntest1", \@empty_args);
    #TestExec::pm_destroy_instance("syntest1", \@empty_args);
}


################################################################################
sub GEN_CONFIG
{
  # scripts/perl_lib/PolicyConfig.pm has an example of how to 
  # set this up after the END tag.

    # Generate a simple Policy_Config.XML.
    #our $pcfg = new PolicyConfig;
    #GEN_CONFIG ($pcfg);
    #our $pcfg_text = $pcfg->config;
    #$ts_config .= $pcfg_text;


  my $cfg = shift;

  my @now = localtime(time);
  my @np8 = localtime(time + (3600 * 8));
  my @np2 = localtime(time + (3600 * 2));
  my @nm8 = localtime(time - (3600 * 8));
  my @nm2 = localtime(time - (3600 * 2));
  
  my $c_any = $cfg->CRITERIA (
         type   => 'dest_domain',
         method => 'domain',
         value  => '.');
 
  my $c_min8 = $cfg->CRITERIA (
         type   => 'time',
         method => 'range',
         value  => qq($nm8[2]:$nm8[1]-$nm2[2]:$nm2[1]));

  my $c_mid8 = $cfg->CRITERIA (
         type   => 'time',
         method => 'range',
         value  => qq($nm2[2]:$nm2[1]-$np2[2]:$np2[1]));

  my $c_plu8 = $cfg->CRITERIA (
         type   => 'time',
         method => 'range',
         value  => qq($np2[2]:$np2[1]-$np8[2]:$np8[1]));

  my $r_any  = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: fall-through"] );
  my $r_min8 = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: past"] );
  my $r_plu8 = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: future"] );
  my $r_mid8 = $cfg->RULE_DATA ( ["add-req-header", 
				  "X-Deft-Policy: current"] );

  $cfg->KEY ('past', $c_min8);
  $cfg->KEY ('future', $c_plu8);
  $cfg->KEY ('current', $c_mid8);
  $cfg->KEY ('fall-through',  $c_any);

  my $r_min8 = $cfg->RULE (
         keyId => 'past',
         auzn  => 'allow-cfg',
         ruleData => $r_min8);

  my $r_mid8 = $cfg->RULE (
         keyId => 'current',
         auzn  => 'allow-cfg',
         ruleData => $r_mid8);

  my $r_plu8 = $cfg->RULE (
         keyId => 'future',
         auzn  => 'allow-cfg',
         ruleData => $r_plu8);

  my $r_any = $cfg->RULE (
         keyId => 'fall-through',
         auzn  => 'allow-cfg',
         ruleData => $r_any);

  $cfg->ACL ("TE", $r_min8, $r_mid8, $r_plu8, $r_any);
};
