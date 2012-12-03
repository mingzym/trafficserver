#!/bin/sh
#
#  run_test.sh
#   Author          : Mike Chowla
#
#   Description:
#
#   $Id: run_test.sh,v 1.2 2003-06-01 18:38:29 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

RUN_TEST="run_test.pl"

if [ -x $RUN_TEST ]; then
    echo "WARNING: run_test.sh deprecated.  Use run_test.pl instead"
    exec $RUN_TEST ${@}
else
    echo "Error: Can not find $RUN_TEST"
    exit 1
fi
