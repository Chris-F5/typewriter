#!/bin/python3

# pager.py
# Read _content_ from standard input, split into _pages_ which are written to
# standard output.

import sys, argparse, re
from utils import *

# Return next non-empty line from _file_.
def next_line(file):
  # Loop until a good line is found.
  while True:
    line = file.readline()
    # If we reach the end of the file then return None.
    if not line:
      return None
    # If this line contains non-whitespace characters then return it.
    if len(line.split()) > 0:
      return line

# A Graphic as might appear in a _pages_ file.
class Graphic:
  # Read a graphic string from _file_.
  def __init__(self, file):
    # _self.string_ is the entire multi-line graphic string.
    self.string = ""
    # A graphic must begin with a START command.
    line = next_line(file)
    if not line or not Graphic.is_start(line):
      warn("Expected START at beginning of graphic.")
      exit(1)
    self.string += line
    # For each START, an opposing END is required to close the graphic.
    depth = 1
    while True:
      line = next_line(file)
      # If we reached the end of file before the graphic is closed.
      if not line:
        warn("Graphic was not ended.")
        exit(1)
      # Add this line to the graphic string.
      self.string += line
      # If we encounter a START command then increase the depth.
      # If we encounter an END command then decrease the depth.
      # When depth reaches zero, the graphic is finished because an equal
      # number of start and END commands have been read.
      if Graphic.is_start(line):
        depth += 1
      elif Graphic.is_end(line):
        depth -= 1
        if depth == 0:
          break
  # Check if this line is a START graphic command.
  def is_start(line):
    # If the first field is START
    fields = line.split()
    if len(fields) < 1:
      return False
    return fields[0] == "START" or fields[0] == '"START"'
  # Check if this line is an END graphic command.
  def is_end(line):
    # If the first field is END
    fields = line.split()
    if len(fields) < 1:
      return False
    return fields[0] == "END" or fields[0] == '"END"'

# Box and Glue are POLYMORPHIC classes, each are a type of gizmo.
# They both implement is_discardable, get_height, is_visible and print.
# Discardable gizmos found at the end of a flow are discarded.
# Visible gizmos will print a graphic.
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
  # _discardable_height_ is the height of all discardable gizmos that are not
  # followed by a non-discardable gizmo.
  discardable_height = 0
  for gizmo in gizmos:
    if gizmo.is_discardable():
      discardable_height += gizmo.get_height()
    else:
      # When a non-discardable gizmos is encountered, its height is added and
      # any previous discardable gizmos are now also added because they are
      # followed by a non-discardable gizmo.
      height += discardable_height
      height += gizmo.get_height()
      discardable_height = 0
  return height

# _PageGenerator_ is responsible for generating new Pages and assigning page
# numbers.
class PageGenerator:
  def __init__(self, width, height, top_padding, bot_padding, left_padding,
               right_padding, header_text):
    self.width = width
    self.height = height
    self.top_padding = top_padding
    self.bot_padding = bot_padding
    self.left_padding = left_padding
    self.right_padding = right_padding
    self.header_text = header_text
    self.page_count = 0
  def new_page(self):
    # Each time I make a new page, its page number is one more.
    self.page_count += 1
    return Page(self.width, self.height, self.top_padding, self.bot_padding,
                self.left_padding, self.right_padding, str(self.page_count),
                self.header_text)

# Stores the content of a single page.
class Page:
  def __init__(self, width, height, top_padding, bot_padding, left_padding,
               right_padding, page_number, header_text):
    self.width = width
    self.height = height
    self.top_padding = top_padding
    self.bot_padding = bot_padding
    self.left_padding = left_padding
    self.right_padding = right_padding
    self.page_number = page_number
    # _max_content_height_ is the height available for this page's content.
    self.max_content_height = self.height - self.top_padding - self.bot_padding
    self.header_text = header_text
    # _normal_gizmos_ and _footnote_gizmos_ are each a 'flow'.
    self.normal_gizmos = []
    self.footnote_gizmos = []
    # _marks_ is a list of mark strings that appear on this page.
    self.marks = []
    self.empty = True
  def mark(self, mark):
    # Mark this page with a string 'mark'. This will be added to the contents
    # file which is used for generating a contents page.
    self.marks.append(mark)
  def write_marks(self, file):
    # Marks are written to _contents_ files after all pages have been
    # generated.
    for mark in self.marks:
      file.write('"{}" "{}"\n'.format(strip_string(mark), \
          strip_string(self.page_number)))
  # Force add gizmo content to this page.
  # _normal_gizmos_ is a list of gizmos in the normal flow to append.
  # _footnote_gizmos_ is a list of gizmos in the footnote flow to append.
  def add_content(self, normal_gizmos, footnote_gizmos):
    # If there is nothing to add then do nothing.
    if len(normal_gizmos) == 0 and len(footnote_gizmos) == 0:
      return
    # The page is no-longer empty.
    self.empty = False
    # Append the gizmos to this page's flow.
    self.normal_gizmos += normal_gizmos
    self.footnote_gizmos += footnote_gizmos
  # Try to add gizmo content. Only succeeds if there is sufficient space on
  # this page.
  # If there is not enough space for ALL gizmos then NO gizmos are added.
  # Return True if successfully added.
  def try_add_content(self, normal_gizmos, footnote_gizmos):
    # _new_used_height_ is how tall the page content would be if the gizmos
    # were added.
    new_used_height = 0
    # Add the height of the potential new normal and footnote flows.
    new_used_height += gizmos_height(self.normal_gizmos + normal_gizmos)
    new_used_height += gizmos_height(self.footnote_gizmos + footnote_gizmos)
    # If the gizmos do not fit and the page is not empty then return False.
    # If the gizmos do not fit but the page is empty then add them anyway
    # because if they do not fit here then they will not fit anywhere and it
    # would cause an infinite loop that keeps adding new empty pages and trying
    # to fit the impossible content in each one.
    if new_used_height > self.max_content_height and not self.empty:
      return False
    # Force add the new gizmos to this page.
    self.add_content(normal_gizmos, footnote_gizmos)
    return True
  # Return the _pages_ graphic string for the page number.
  def page_number_graphic(self):
    # Build the _text_specification_ for the page number.
    text = "FONT Regular 12\n"
    text += 'STRING "{}"\n'.format(strip_string(self.page_number))
    # line_break is called to centre the text.
    graphic = line_break(text, \
        self.width - self.left_padding - self.right_padding, 'c')
    # Remove pager commands from the _content_ to turn it into a graphic that
    # can be inserted into _pages_.
    graphic = re.sub(r"opt_break.*", "", graphic)
    graphic = re.sub(r"box.*", "", graphic)
    return graphic
  # Return the _pages_ graphic string for the page header.
  def header_graphic(self):
    # Build the _text_specification_ for the header.
    text = "FONT Italic 12\n"
    text += 'STRING "{}"\n'.format(strip_string(self.header_text))
    # Break the header into lines.
    graphic = line_break(text, \
        self.width - self.left_padding - self.right_padding, 'c')
    # Remove pager commands from the _content_ to turn it into a graphic that
    # can be inserted into _pages_.
    graphic = re.sub(r"opt_break.*", "", graphic)
    graphic = re.sub(r"box.*", "", graphic)
    return graphic
  # Write this page's _pages_ content to standard output.
  def print(self, show_page_number):
    # Start the page.
    print("START PAGE")
    # Start at the top of the page minus the top padding.
    y = self.height - self.top_padding
    x = self.left_padding
    # For each gizmo in the normal flow.
    for gizmo in self.normal_gizmos:
      # Move down the page by this gizmo's height.
      y -= gizmo.get_height()
      # If the gizmo is visible then move to (x, y) and print it.
      if gizmo.is_visible():
        print("MOVE {} {}".format(x, y))
        gizmo.print()
    # Now set _y_ to the bottom of the page plus the bottom padding plus the
    # total footnote flow height.
    y = self.bot_padding + gizmos_height(self.footnote_gizmos)
    # For each gizmo in the footnote flow.
    for gizmo in self.footnote_gizmos:
      # Move down by this gizmos height.
      y -= gizmo.get_height()
      # If the gizmo is visible then move to (x, y) and print it.
      if gizmo.is_visible():
        print("MOVE {} {}".format(x, y))
        gizmo.print()
    # If page numbers are enabled then move to the bottom of the page and write
    # the page number graphic.
    if show_page_number:
      print("MOVE {} {}".format(self.left_padding, self.bot_padding // 2))
      print(self.page_number_graphic())
    # If the header is not empty then move to the top of the page and print it.
    if self.header_text != "":
      print("MOVE {} {}".format(self.left_padding, self.height - self.top_padding // 2))
      print(self.header_graphic())
    # End the page.
    print("END")

# Parse pager command line arguments.
arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-l", "--left_margin", type=int, default=102)
arg_parser.add_argument("-r", "--right_margin", type=int, default=102)
arg_parser.add_argument("-t", "--top_margin", type=int, default=125)
arg_parser.add_argument("-b", "--bot_margin", type=int, default=125)
# -c / --contents Where to output the contents file. Default is None.
arg_parser.add_argument("-c", "--contents") 
# If the -n flag is present then page numbers will be drawn.
arg_parser.add_argument("-n", "--page_numbers", action="store_true")
arg_parser.add_argument("-H", "--header", default="")
args = arg_parser.parse_args()

# 595x842 is the point resolution of an A4 page.
page_generator = PageGenerator(595, 842, args.top_margin, args.bot_margin, \
    args.left_margin, args.right_margin, args.header)
pages = []
# Generate an initial page.
active_page = page_generator.new_page()
# Pending gizmos are stored in a buffer. When an (optional) page break is read,
# they are removed from this buffer and added to a Page object.
pending_gizmos = {"normal": [], "footnote": []}
current_flow = "normal"
# For each record parsed in this programs standard input.
while fields := parse_record(sys.stdin):
  # The first field in the record is the pager command type.
  if fields[0] == "flow":
    # flow [normal/footnote]
    if len(fields) != 2:
      warn("flow command expects one argument.")
      continue # Go to the next record.
    # If the argument of the flow command is not "normal" or "footnote":
    if not fields[1] in pending_gizmos.keys():
      warn("invalid flow '{}'".format(fields[1]))
      continue
    # Update the _current_flow_ to the argument of this flow command.
    current_flow = fields[1]
  elif fields[0] == "mark":
    # mark MARK_STRING
    if len(fields) != 2:
      warn("mark command expects one argument.")
      continue
    # Mark this page with the string argument of the mark command.
    active_page.mark(fields[1])
  elif fields[0] == "box":
    # box GRAPHIC_HEIGHT
    # [pages graphic]
    if len(fields) != 2:
      warn("box command expects one argument.")
      continue
    # Try to convert the argument of the command to an integer. If it does not
    # work then just default to a height of zero.
    try:
      height = int(fields[1])
    except:
      warn("box command argument must be integer.")
      height = 0
    # Box command is followed by a pages graphics so read this graphic and
    # store it in _graphic_.
    graphic = Graphic(sys.stdin)
    # Add this Box to the pending gizmos in the current flow.
    box = Box(height, graphic)
    pending_gizmos[current_flow].append(box)
  elif fields[0] == "glue":
    # glue GLUE_HEIGHT
    if len(fields) != 2:
      warn("glue command expects one argument.")
      continue
    # Try to convert the argument of the command to an integer. If it does not
    # work then just default to a height of zero.
    try:
      height = int(fields[1])
    except:
      warn("glue command argument must be integer.")
      height = 0
    # Add this glue to the pending gizmos of the current flow.
    glue = Glue(height)
    pending_gizmos[current_flow].append(glue)
  elif fields[0] == "opt_break" or fields[0] == "new_page":
    # opt_break and new_page have no arguments.
    # They both require flushing the pending gizmos.
    # If the pending gizmos wont fit on the current page:
    if not active_page.try_add_content(pending_gizmos["normal"],
                                pending_gizmos["footnote"]):
      # Then make a new page and add the gizmos onto that.
      pages.append(active_page)
      active_page = page_generator.new_page()
      active_page.add_content(pending_gizmos["normal"],
                              pending_gizmos["footnote"])
    # The pending gizmos have now been flushed so _pending_gizmos_ is reset.
    pending_gizmos = {"normal": [], "footnote": []}
    # If we wanted a new page and the current page is not empty then make a new
    # page.
    if fields[0] == "new_page" and not active_page.empty:
      pages.append(active_page)
      active_page = page_generator.new_page()
  else:
    # Good example of error handling.
    warn("unrecognised command '{}'".format(fields[0]))

# Flush left-over pending gizmos.
if not active_page.try_add_content(pending_gizmos["normal"],
                            pending_gizmos["footnote"]):
  pages.append(active_page)
  active_page = page_generator.new_page()
  active_page.add_content(pending_gizmos["normal"],
                          pending_gizmos["footnote"])
pages.append(active_page)

# If the user wanted a contents file then make it.
contents = None
if args.contents:
  contents = open(args.contents, "w")
# Loop over all pages.
for page in pages:
  # Print the _pages_ content to standard output.
  page.print(args.page_numbers)
  # If we are using a contents file then write this page's marks to it.
  if contents:
    page.write_marks(contents)
# If we used a contents file then close it.
if contents:
  contents.close()
