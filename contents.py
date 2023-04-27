#!/bin/python3

# contents.py
# Generates contents page's content.
# Parses standard input in _contents_ format and writes _content_ to standard
# output.

import sys, argparse
from utils import *

# Parse command line arguments.
arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-c", "--char_width", type=int, default=60)
arg_parser.add_argument("-s", "--font_size", type=int, default=12)
args = arg_parser.parse_args()

char_width = args.char_width
font_size = args.font_size

# Keep parsing records from standard input until reach end.
# fields is an array of fields in this record.
while fields := parse_record(sys.stdin):
  if len(fields) != 2:
    warn("contents file record must have 2 fields")
    continue
  # _padding_ is how many dots we need to separate the section name from the
  # page number.
  padding = char_width - len(fields[0]) - len(fields[1])
  # If the text overflows the width then don't add any dots.
  if padding < 0:
    padding = 0
  # Build the _content_ text for this line.
  # A monospaced font is used to make dots align in each line of
  # the contents page.
  graphic = "box {}\n".format(args.font_size)
  graphic += "START TEXT\n"
  graphic += "FONT Monospace {}\n".format(args.font_size)
  string = fields[0] + '.' * padding + fields[1]
  # _strip_string_ removes illegal characters from the string.
  graphic += 'STRING "{}"\n'.format(strip_string(string))
  graphic += "END\n"
  # A page break may occur in-between contents lines.
  graphic += "opt_break\n"
  # Write this line's _content_ to standard output.
  print(graphic, end='')
