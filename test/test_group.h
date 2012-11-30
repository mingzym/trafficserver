/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   test_group.h
   Author          : Mike Chowla

   Description:

   $Id: test_group.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/


#ifndef _TEST_GROUP_H_
#define _TEST_GROUP_H_


int load_group_file(const char* filename);

struct test_group_iter;

struct test_case {
    test_case();
    ~test_case();
    char* name;
    const char** test_case_elements;
};

test_group_iter* test_group_start(const char* tg_name);
const test_case* test_group_next(test_group_iter* tg_iter);
void test_group_finish(test_group_iter* tg_iter);

int lookup_test_case(const char* name, test_case* result);

#endif
