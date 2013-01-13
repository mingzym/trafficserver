#!/usr/bin/python
#
#  parse_test_log.py
#  Author          : Mike Chowla
#
#   Description:
#
#   $Id: parse_test_log.pl,v 1.2 2003-06-01 18:38:30 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

##use Vars;
import os
import sys
import argparse
import re
from warnings import warn

import parse_dispatcher;


parser = argparse.ArgumentParser()
parser.add_argument('--in_file', '-i', required=True, help='the file to parse',
  dest='in_file')
parser.add_argument('--out_file', '-o', help='the file to write, default to STDOUT',
  dest='out_file')
parser.add_argument('--test_name', '-t', default='Unknown', help='test script name',
  dest='test_name')
parser.add_argument('--html', '-H', action='store_true', default=False, 
  dest='output_html')

args = parser.parse_args()

log_in = open(args.in_file, 'r')
if not log_in:
  raise Exception("Could not open log file " + in_file + " : $!\n")

if args.out_file:
  log_out = open(args.out_file, 'w')
  if not log_out:
    raise Exception("Could not open output file " + out_file + " : $!\n")
else:
  log_out = sys.stdout


tmp_body = ""
tmp_summary = ""
errors = 0
warnings = 0

if args.output_html:
    tmp_body = open(args.out_file + ".tmp_body", "w")
    if not tmp_body:
      raise Exception("Could not open tmp file " + mp_body + " : $!\n")
    tmp_summary = open(args.out_file + ".tmp_summary", "w")
    if not tmp_summary:
      raise Exception("Could not open tmp file " + tmp_summary + " : $!\n")


problem_count = 0
for line in log_in:

  r = parse_dispatcher.process_test_log_line(line)

  if r != "" and  r != "ok":
    if r == "error":
      errors += 1
      problem_count += 1
    elif r == "warning":
      warnings += 1
      problem_count += 1
    else:
      warn("unknown line type from " + r + "\n")

    if args.output_html:
      line = line.rstrip('\r\n')
      tmp_summary.write('<a href="#problem_' + str(problem_count) + '">' + r + ": " + line  + "</a>\n")
    
      problem_number = problem_count + 1
      body_str = '<a name="problem_' + str(problem_count) + '" ' + 'href="#problem_' + str(problem_number) + '">NEXT </a> '
      if r == 'error':
        body_str = body_str + "<font color=\"red\">"
      else:
        body_str = body_str + "<font color=\"purple\">"
      body_str = body_str + line + "</font>\n"
      tmp_body.write(body_str)
    else:
      log_out.write(r + ": " + line)

  else:
    if args.output_html:
      line = line.rstrip('\r\n')
      tmp_body.write("      " + line + "\n")

log_in.close()

if args.output_html:
  tmp_body.close()
  tmp_summary.close()

  tmp_body = open(args.out_file + ".tmp_body", "w")
  if not tmp_body:
    raise Exception("Could not open tmp file $tmp_body : $!\n")
  tmp_summary = open(args.out_file + ".tmp_summary", "w")
  if not tmp_summary:
    raise Exception("Could not open tmp file $tmp_summary : $!\n")

  log_out.write("<html>\n<head>\n<title>Test Report for " + args.test_name + " </title>\n</head>\n")
  log_out.write("<body bgcolor=\"White\">\n<h2> Test Report for " + args.test_name + " ")
  log_out.write("</h2>\n<h3> Summary: </h3>")
  log_out.write(" <h4><font color=\"red\">" + str(errors) + " Errors</font>")
  log_out.write("; <font color=\"purple\">" + str(warnings) + "  Warnings</font></h4>\n<pre>\n")

  for tmp in tmp_summary:
    log_out.write(tmp)
  tmp_summary.close()
  os.remove(args.out_file + ".tmp_summary")

  log_out.write("</pre>\n<h3> Full Log </h3>\n<pre>\n")

  for tmp in tmp_body:
    log_out.write(tmp)
    
  tmp_body.close()
  os.remove(args.out_file + ".tmp_body")

  log_out.write("</pre>\n")

  if problem_count > 0:
    problem_number = problem_count + 1
    log_out.write("<h4><a name=\"#problem_" + str(problem_number) + "\">" +
      "No More Errors </a></h4>\n")

  log_out.write("</body>\n</html>\n")

else:
  log_out.write("\n#### " + str(errors) + " Errors; " + str(warnings) + "  Warnings ####\n")

log_out.close()

if args.out_file:
  print "#### " + str(errors) + " Errors; " + str(warnings) + "  Warnings ####w\n"

sys.exit(0)







