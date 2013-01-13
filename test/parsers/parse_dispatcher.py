#
#  parse_dispatcher.pm
#  Author          : Mike Chowla
#
#   Description:
#
#   $Id: parse_dispatcher.pm,v 1.2 2003-06-01 18:38:30 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

import re
import parse_generic
from warnings import warn

VERSION = 1.00

instance_hash = {}

predefined_mappings={"syntest":"parse_syntest", "jtest":"parse_jtest", "ts": "parse_ts"}

def load_parser_module(parser_name):
  module_load_str = parser_name + ".py"

  try:
    import module_load_str
  except:
    print "Unable to load module " + module_load_str + " : $@\n"
    return 1
  return 0

def process_test_log_line(line):
  r = "unknown"

  m = re.search("/^\[\w{3} \w{3}\s{1,2}\d{1,2} \d{1,2}:\d{1,2}:\d{1,2}\.\d{1,3} ([^\]]+) ([^\]]+)\]\s+(.*)/", line)
  if m:
    instance, level, rest_of_line = m.groups()

    if instance == "log_parse" and level == "directive":
      directive, d_instance, d_parser = rest_of_line.split(None, 2)
      if directive == "log-parser-set":
        load_result = load_parser_module(d_parser)
        if load_result == 0:
          instance_hash[d_instance] = d_parser
          r = "ok"
        else:
          r = "error"
      else:
        warn("bad directive sent to log_parse\n")
        r = "warning"

    else:
      if instance_hash[instance] :
        found_module = 0
        m = re.search('^(\D+)\d+$', instance)
        if m:
          mid = m.groups()
          if predefined_mappings[mid]:
            module_str = predefined_mappings[mid]
            if module_str:
              load_result = load_parser_module(module_str)
              if load_result == 0:
                instance_hash[instance] = module_str
                found_module = 1

  	    if found_module == 0 :
		      instance_hash[instance] = "parse_generic"

	    module_name = instance_hash[instance]
	    cmd = 'r = ' + module_name + '.' + 'process_test_log_line(instance, level,  rest_of_line)'
      try:
        exec(cmd)
      except:
        warn("##### eval failed: $@\n")

  return r


