#
#  parse_jtest.pm
#  Author          : Mike Chowla
#
#   Description:
#
#   $Id: parse_generic.pm,v 1.2 2003-06-01 18:38:30 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

import re

VERSION = 1.00

def process_test_log_line(instance_id, level, line):
  if re.search("error", line, re.I) or re.search("Abort", line, re.I) or re.search("Fatal", line, re.I) or re.search("error", level, re.I) :
    return "error"
  elif re.search("warning", line, re.I) or re.search("warning", level, re.I) :
    return "warning"
  elif level == "stderr":
    return "error"
  else:
    return "ok"
