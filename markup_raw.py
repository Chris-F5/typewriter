#!/bin/python3

# markup_raw.py
# Read text from standard input, preserve line breaks and write _content_ text
# to standard output with optional breaks inbetween lines.

import sys, argparse
from utils import *

# Orphans are isolated lines of text at the bottom of a page.
# Widows are isolated lines of text at the top of a page.

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-s", "--font_size", type=int, default=12)
arg_parser.add_argument("-f", "--font_name", default="Monospace")
arg_parser.add_argument("-o", "--orphans", type=int, default=1)
arg_parser.add_argument("-w", "--widows", type=int, default=1)
args = arg_parser.parse_args()

font_size = args.font_size
font = args.font_name
# _orphans_ is the maximum number of allowed orphans.
orphans = args.orphans
# _widows_ is the maximum number of allowed widowsd.
widows = args.widows

# Read all lines from standard input and loop over them.
lines = sys.stdin.readlines()
i = 0
for line in lines:
  i += 1
  # Make a box for this line with the line text in it and write this to stdout.
  print("box {}".format(font_size))
  print("START TEXT")
  print("FONT {} {}".format(strip_string(font), font_size))
  print('STRING "{}"'.format(strip_string(line)))
  print("END")
  # Only allow a page break here if it does not result in too many orphans or
  # widows.
  if i >= orphans and len(lines) - i >= widows:
    print("opt_break")
# Allow a page break after all lines of text.
print("opt_break")
