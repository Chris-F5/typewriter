#!/bin/python3

import sys, subprocess, re

def warn(msg):
  print(msg, file=sys.stderr)

def line_break(text, width, align):
  process = subprocess.Popen(["./line_break", "-" + align, "-w", str(width)],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE)
  text = bytes(text, "ascii")
  process.stdin.write(text)
  output, error = process.communicate()
  return output.decode("ascii")

def parse_record(line):
  fields = re.findall(r'[^"\s]\S*|".*?[^\\]"', line)
  for i in range(len(fields)):
    if fields[i][0] == '"':
      fields[i] = fields[i][1:-1]
    fields[i] = fields[i].replace('\\"', '"')
  return fields

def strip_string(string):
  return string.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '')

char_width = 60

for line in sys.stdin:
  fields = parse_record(line)
  if len(fields) != 2:
    warn("contents file record must have 2 fields")
    continue
  padding = char_width - len(fields[0]) - len(fields[1])
  if padding < 0:
    padding = 0
  graphic = "box 12\n"
  graphic += "START TEXT\n"
  graphic += "FONT Monospace 12\n"
  string = fields[0] + '.' * padding + fields[1]
  graphic += 'STRING "{}"\n'.format(strip_string(string))
  graphic += "END\n"
  graphic += "opt_break\n"
  print(graphic, end='')
