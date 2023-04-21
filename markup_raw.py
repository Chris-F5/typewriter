#!/bin/python3

import sys, argparse
from utils import *

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-s", "--font_size", type=int, default=12)
arg_parser.add_argument("-f", "--font_name", default="Monospace")
arg_parser.add_argument("-o", "--orphans", type=int, default=1)
arg_parser.add_argument("-w", "--widows", type=int, default=1)
args = arg_parser.parse_args()

font_size = args.font_size
font = args.font_name
orphans = args.orphans
widows = args.widows

lines = sys.stdin.readlines()

i = 0
for line in lines:
  i += 1
  print("box {}".format(font_size))
  print("START TEXT")
  print("FONT {} {}".format(strip_string(font), font_size))
  print('STRING "{}"'.format(strip_string(line)))
  print("END")
  if i >= orphans and len(lines) - i >= widows:
    print("opt_break")
print("opt_break")
