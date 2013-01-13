#
#  parse_jtest.pm
#
# TBD

import re

def process_test_log_line(instance_id, level, line):
  if re.search("error", line, re.I) and level == "stderr":
    return "error";
  elif re.search("warn", line, re.I):
    return "warning"
  else:
    return "ok"

