#!/bin/python3

import sys, subprocess, re, argparse

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

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-c", "--char_width", type=int, default=60)
arg_parser.add_argument("-s", "--font_size", type=int, default=12)
args = arg_parser.parse_args()

char_width = args.char_width
font_size = args.font_size

for line in sys.stdin:
  fields = parse_record(line)
  if len(fields) != 2:
    warn("contents file record must have 2 fields")
    continue
  padding = char_width - len(fields[0]) - len(fields[1])
  if padding < 0:
    padding = 0
  graphic = "box {}\n".format(args.font_size)
  graphic += "START TEXT\n"
  graphic += "FONT Monospace {}\n".format(args.font_size)
  string = fields[0] + '.' * padding + fields[1]
  graphic += 'STRING "{}"\n'.format(strip_string(string))
  graphic += "END\n"
  graphic += "opt_break\n"
  print(graphic, end='')
