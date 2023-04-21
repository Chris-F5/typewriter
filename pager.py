#!/bin/python3

import sys, argparse, re
from utils import *

def next_line(file):
  while True:
    line = file.readline()
    if not line:
      break
    if len(line.split()) > 0:
      break
  return line

class Graphic:
  def __init__(self, file):
    self.string = ""
    depth = 1
    line = next_line(file)
    if not line or not Graphic.is_start(line):
      warn("Expected START at beginning of graphic.")
      exit(1)
    self.string += line
    while True:
      line = next_line(file)
      if not line:
        warn("Graphic was not ended.")
        exit(1)
      self.string += line
      if Graphic.is_start(line):
        depth += 1
      elif Graphic.is_end(line):
        depth -= 1
        if depth == 0:
          break
  def is_start(line):
    fields = line.split()
    if len(fields) < 1:
      return False
    return fields[0] == "START" or fields[0] == '"START"'
  def is_end(line):
    fields = line.split()
    if len(fields) < 1:
      return False
    return fields[0] == "END" or fields[0] == '"END"'

class Box:
  def __init__(self, height, graphic):
    self.height = height
    self.graphic = graphic
  def is_discardable(self):
    return False
  def get_height(self):
    return self.height
  def is_visible(self):
    return True
  def print(self):
    print(self.graphic.string, end='')
class Glue:
  def __init__(self, height):
    self.height = height
  def is_discardable(self):
    return True
  def get_height(self):
    return self.height
  def is_visible(self):
    return False
  def print(self):
    pass

def gizmos_height(gizmos):
  height = 0
  discardable_height = 0
  for gizmo in gizmos:
    if gizmo.is_discardable():
      discardable_height += gizmo.get_height()
    else:
      height += discardable_height
      height += gizmo.get_height()
      discardable_height = 0
  return height

class PageGenerator:
  def __init__(self, width, height, top_padding, bot_padding, left_padding,
               right_padding):
    self.width = width
    self.height = height
    self.top_padding = top_padding
    self.bot_padding = bot_padding
    self.left_padding = left_padding
    self.right_padding = right_padding
    self.page_count = 0
  def new_page(self):
    self.page_count += 1
    return Page(self.width, self.height, self.top_padding, self.bot_padding,
                self.left_padding, self.right_padding, str(self.page_count))

class Page:
  def __init__(self, width, height, top_padding, bot_padding, left_padding,
               right_padding, page_number):
    self.width = width
    self.height = height
    self.top_padding = top_padding
    self.bot_padding = bot_padding
    self.left_padding = left_padding
    self.right_padding = right_padding
    self.page_number = page_number
    self.max_content_height = self.height - self.top_padding - self.bot_padding
    self.normal_gizmos = []
    self.footnote_gizmos = []
    self.marks = []
    self.empty = True
  def mark(self, mark):
    self.marks.append(mark)
  def write_marks(self, file):
    for mark in self.marks:
      file.write('"{}" "{}"\n'.format(strip_string(mark), \
          strip_string(self.page_number)))
  def add_content(self, normal_gizmos, footnote_gizmos):
    if len(normal_gizmos) == 0 and len(footnote_gizmos) == 0:
      return
    self.empty = False
    self.normal_gizmos += normal_gizmos
    self.footnote_gizmos += footnote_gizmos
  def try_add_content(self, normal_gizmos, footnote_gizmos):
    new_used_height = 0
    new_used_height += gizmos_height(self.normal_gizmos + normal_gizmos)
    new_used_height += gizmos_height(self.footnote_gizmos + footnote_gizmos)
    if new_used_height > self.max_content_height and not self.empty:
      return False
    self.add_content(normal_gizmos, footnote_gizmos)
    return True
  def page_number_graphic(self):
    text = "FONT Regular 12\n"
    text += 'STRING "{}"'.format(strip_string(self.page_number))
    graphic = line_break(text, \
        self.width - self.left_padding - self.right_padding, 'c')
    graphic = re.sub(r"opt_break.*", "", graphic)
    graphic = re.sub(r"box.*", "", graphic)
    return graphic
  def print(self, show_page_number):
    print("START PAGE")
    y = self.height - self.top_padding
    x = self.left_padding
    for gizmo in self.normal_gizmos:
      y -= gizmo.get_height()
      if gizmo.is_visible():
        print("MOVE {} {}".format(x, y))
        gizmo.print()
    y = self.bot_padding + gizmos_height(self.footnote_gizmos)
    for gizmo in self.footnote_gizmos:
      y -= gizmo.get_height()
      if gizmo.is_visible():
        print("MOVE {} {}".format(x, y))
        gizmo.print()
    if show_page_number:
      print("MOVE {} {}".format(self.left_padding, self.bot_padding // 2))
      print(self.page_number_graphic())
    print("END")

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-l", "--left_margin", type=int, default=102)
arg_parser.add_argument("-r", "--right_margin", type=int, default=102)
arg_parser.add_argument("-t", "--top_margin", type=int, default=125)
arg_parser.add_argument("-b", "--bot_margin", type=int, default=125)
arg_parser.add_argument("-c", "--contents")
arg_parser.add_argument("-n", "--page_numbers", action="store_true")
args = arg_parser.parse_args()

page_generator = PageGenerator(595, 842, args.top_margin, args.bot_margin, \
    args.left_margin, args.right_margin)
pages = []
active_page = page_generator.new_page()
pending_normal_gizmos = []
pending_footnote_gizmos = []
pending_gizmos = {"normal": [], "footnote": []}
current_flow = "normal"
while fields := parse_record(sys.stdin):
  if fields[0] == "flow":
    if len(fields) != 2:
      warn("flow command expects one argument.")
      continue
    if not fields[1] in pending_gizmos.keys():
      warn("invalid flow '{}'".format(fields[1]))
      continue
    current_flow = fields[1]
  elif fields[0] == "mark":
    if len(fields) != 2:
      warn("mark command expects one argument.")
      continue
    active_page.mark(fields[1])
  elif fields[0] == "box":
    if len(fields) != 2:
      warn("box command expects one argument.")
      continue
    try:
      height = int(fields[1])
    except:
      warn("box command argument must be integer.")
      height = 0
    graphic = Graphic(sys.stdin)
    box = Box(height, graphic)
    pending_gizmos[current_flow].append(box)
  elif fields[0] == "glue":
    if len(fields) != 2:
      warn("glue command expects one argument.")
      continue
    try:
      height = int(fields[1])
    except:
      warn("glue command argument must be integer.")
      height = 0
    glue = Glue(height)
    pending_gizmos[current_flow].append(glue)
  elif fields[0] == "opt_break" or fields[0] == "new_page":
    if not active_page.try_add_content(pending_gizmos["normal"],
                                pending_gizmos["footnote"]):
      pages.append(active_page)
      active_page = page_generator.new_page()
      active_page.add_content(pending_gizmos["normal"],
                              pending_gizmos["footnote"])
    pending_gizmos = {"normal": [], "footnote": []}
    if fields[0] == "new_page" and not active_page.empty:
      pages.append(active_page)
      active_page = page_generator.new_page()
  else:
    warn("unrecognised command '{}'".format(fields[0]))

if not active_page.try_add_content(pending_gizmos["normal"],
                            pending_gizmos["footnote"]):
  pages.append(active_page)
  active_page = page_generator.new_page()
  active_page.add_content(pending_gizmos["normal"],
                          pending_gizmos["footnote"])

contents = None
if args.contents:
  contents = open(args.contents, "w")
pages.append(active_page)
for page in pages:
  page.print(args.page_numbers)
  if contents:
    page.write_marks(contents)
if contents:
  contents.close()
