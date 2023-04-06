#!/bin/python3

import sys, argparse

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-s", "--font_size", type=int, default=12)
arg_parser.add_argument("-f", "--font_name", default="Monospace")
args = arg_parser.parse_args()

font_size = args.font_size
font = args.font_name

for line in sys.stdin:
  line = line.replace('\n', '')
  print("box {}".format(font_size))
  print("START TEXT")
  print("FONT {} {}".format(font, font_size))
  print('STRING "{}"'.format(line.replace('\\', '\\\\').replace('"', '\\"')))
  print("END")
  print("opt_break")
