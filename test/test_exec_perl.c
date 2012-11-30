/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   test_exec_perl.c
   Author          : Mike Chowla

   Description:

   $Id: test_exec_perl.c,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#include <EXTERN.h>
#include "perl.h"

static void xs_init (pTHX);

EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);
EXTERN_C void boot_TestExec (pTHX_ CV* cv);

EXTERN_C void
xs_init(pTHX)
{
    char *file = __FILE__;
    /* DynaLoader is a special case */
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
    newXS("TestExec::boot_TestExec", boot_TestExec, file);
}


static PerlInterpreter *my_perl;

void run_perl(char** argv) {

    char** tmp = argv;
    int argc = 0;

    while (*tmp != NULL) {
	tmp++;
	argc++;
    }

    my_perl = perl_alloc();
    perl_construct(my_perl);

    perl_parse(my_perl, xs_init, argc, argv, NULL);
    perl_run(my_perl);

    perl_destruct(my_perl);
    perl_free(my_perl);

}

