#!/bin/python3

# contents.py
# Generated contents page's content.
# Parses standard input in _contents_ format and writes _content_ to standard
# output.

import sys, re, argparse
from utils import *

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-c", "--char_width", type=int, default=60)
arg_parser.add_argument("-s", "--font_size", type=int, default=12)
args = arg_parser.parse_args()

char_width = args.char_width
font_size = args.font_size

while fields := parse_record(sys.stdin):
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
