#!/bin/python3

# pager.py
# Read _content_ from standard input, split into _pages_ which are written to
# standard output.

import sys, argparse, re
from utils import *

# Return next non-empty line from _file_.
def next_line(file):
  while True:
    line = file.readline()
    if not line:
      break
    if len(line.split()) > 0:
      break
  return line

# A Graphic as might appear in a _pages_ file.
class Graphic:
  def __init__(self, file):
    # _self.string_ is the entire multi-line graphic string.
    self.string = ""
    depth = 1
    # A graphic must begin with a START command.
    line = next_line(file)
    if not line or not Graphic.is_start(line):
      warn("Expected START at beginning of graphic.")
      exit(1)
    self.string += line
    # For each START, an opposing END is required to close the graphic.
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

# Box and Glue are polymorphic classes, each are a type of gizmo.
# _discardable_ gizmos found at the end of a flow are discarded.
# _visible_ gizmos will print a graphic.
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

# Given a list of gizmos, compute the height of this flow.
# Trailing discardable gizmos are discarded.
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

# _PageGenerator_ is responsible for generating new Pages and assigning page
# numbers.
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
    # _normal_gizmos_ and _footnote_gizmos_ are each a flow.
    self.normal_gizmos = []
    self.footnote_gizmos = []
    # _marks_ is a list of mark strings that appear on this page.
    self.marks = []
    self.empty = True
  def mark(self, mark):
    self.marks.append(mark)
  def write_marks(self, file):
    # Marks are written to _contents_ files.
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
    # Add the gizmos so long as they ALL fit on this page.
    # Return True if successfully added.
    new_used_height = 0
    new_used_height += gizmos_height(self.normal_gizmos + normal_gizmos)
    new_used_height += gizmos_height(self.footnote_gizmos + footnote_gizmos)
    if new_used_height > self.max_content_height and not self.empty:
      return False
    self.add_content(normal_gizmos, footnote_gizmos)
    return True
  def page_number_graphic(self):
    # Return the _pages_ graphic string for the page number.
    text = "FONT Regular 12\n"
    text += 'STRING "{}"'.format(strip_string(self.page_number))
    # line_break is called to centre the text.
    graphic = line_break(text, \
        self.width - self.left_padding - self.right_padding, 'c')
    graphic = re.sub(r"opt_break.*", "", graphic)
    graphic = re.sub(r"box.*", "", graphic)
    return graphic
  def print(self, show_page_number):
    # Write the _pages_ content that encodes this page.
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

# 595x842 is the point resolution of an A4 page.
page_generator = PageGenerator(595, 842, args.top_margin, args.bot_margin, \
    args.left_margin, args.right_margin)
pages = []
active_page = page_generator.new_page()
pending_normal_gizmos = []
pending_footnote_gizmos = []
# Pending gizmos are waiting to be added to a page.
pending_gizmos = {"normal": [], "footnote": []}
current_flow = "normal"
while fields := parse_record(sys.stdin):
  for i in range(len(fields)):
    if fields[i][0] == '"':
      fields[i] = fields[i][1:-1]
  if fields[0] == "flow":
    # flow [normal/footnote]
    if len(fields) != 2:
      warn("flow command expects one argument.")
      continue
    if not fields[1] in pending_gizmos.keys():
      warn("invalid flow '{}'".format(fields[1]))
      continue
    current_flow = fields[1]
  elif fields[0] == "mark":
    # mark MARK_STRING
    if len(fields) != 2:
      warn("mark command expects one argument.")
      continue
    active_page.mark(fields[1])
  elif fields[0] == "box":
    # box GRAPHIC_HEIGHT
    # [pages graphic]
    if len(fields) != 2:
      warn("box command expects one argument.")
      continue
    try:
      height = int(fields[1])
    except:
      warn("box command argument must be integer.")
      height = 0
    # Box command must be followed by a pages graphic.
    graphic = Graphic(sys.stdin)
    box = Box(height, graphic)
    pending_gizmos[current_flow].append(box)
  elif fields[0] == "glue":
    # glue GLUE_HEIGHT
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
    # opt_break and new_page have not arguments.
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

# Add left over pending gizmos.
if not active_page.try_add_content(pending_gizmos["normal"],
                            pending_gizmos["footnote"]):
  pages.append(active_page)
  active_page = page_generator.new_page()
  active_page.add_content(pending_gizmos["normal"],
                          pending_gizmos["footnote"])

# Write pages and contents file
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
