/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   test_results.h
   Author          : Mike Chowla

   Description:

   $Id: test_results.h,v 1.2 2003-06-01 18:38:30 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _TEST_RESULTS_H_
#define _TEST_RESULTS_H_

#include <time.h>
#include "List.h"

struct TestRunResults;

struct TestResult {
    TestResult();
    ~TestResult();

    void start(const char* name_arg);
    void finish();
    void build_output_file_name(const char* base,
				const char* ext);

    char* test_case_name;
    char* output_file;
    const TestRunResults* test_run_results;

    int errors;
    int warnings;

    time_t time_start;
    time_t time_stop;

    LINK(TestResult, link);
};

struct TestRunResults {
    TestRunResults();
    ~TestRunResults();

    void start(const char* testcase_name, const char* username, const char* build_id);
    TestResult* new_result();
    void cleanup_results(bool print);

    void build_tinderbox_message_hdr(const char* status,
				     time_t now,
				     sio_buffer* output);
    int post_tinderbox_message(sio_buffer* hdr, sio_buffer* body);
    void send_final_tinderbox_message();

    void build_summary_html(sio_buffer* output);
    int output_summary_html();
    
    char* run_id_str;
    char* test_name;
    char* username;
    char* build_id;
    time_t start_time;
    bool cleanup_called;

    DLL<TestResult> results;
};

#endif
