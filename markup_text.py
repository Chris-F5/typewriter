#!/bin/python3

import sys, argparse
from utils import *

class TextStream:
  def __init__(self, width, align, paragraph_spacing, line_spacing):
    self.width = width
    self.align = align
    self.paragraph_spacing = paragraph_spacing
    self.line_spacing = line_spacing
    self.in_paragraph = False
    self.in_string = False
    self.text = ""
    self.insertions = []
  def add_string(self, string):
    if not self.in_string:
      self.text += 'STRING "'
      self.in_string = True
    self.text += strip_string(string)
  def close_string(self):
    if self.in_string:
      self.text += '"\n'
      self.in_string = False
  def set_font(self, font_name, font_size):
    self.close_string()
    self.text += "FONT {} {}\n".format(font_name, font_size)
  def add_word(self, word):
    if len(word) == 0:
      return
    self.close_string()
    if self.in_paragraph:
      self.text += 'OPTBREAK " " "" {}\n'.format(self.line_spacing)
    self.in_paragraph = True
    self.add_string(word)
  def end_paragraph(self):
    if self.in_paragraph:
      self.close_string()
      self.text += "BREAK {}\n".format(self.paragraph_spacing)
      self.in_paragraph = False
  def insert_content(self, insertion):
    self.close_string()
    mark = len(self.insertions)
    self.insertions.append(insertion)
    self.text += "MARK {}\n".format(mark)
  def to_content(self):
    self.close_string()
    content = line_break(self.text, self.width, self.align)
    for i in range(len(self.insertions)):
      content = content.replace('\n^' + str(i), '\n' + self.insertions[i])
    return content
  def read_words(self, line):
    words = line.split()
    for word in words:
      self.add_word(word)

class MainStream(TextStream):
  def __init__(self, normal_width, footnote_width, normal_size, footnote_size, \
      normal_align, footnote_align, normal_paragraph_spacing, \
      normal_line_spacing, footnote_paragraph_spacing, footnote_line_spacing):
    super().__init__(normal_width, normal_align, normal_paragraph_spacing, \
        normal_line_spacing)
    self.footnote_width = footnote_width
    self.normal_size = normal_size
    self.footnote_size = footnote_size
    self.footnote_align = footnote_align
    self.footnote_line_spacing = footnote_line_spacing
    self.footnote_paragraph_spacing = footnote_paragraph_spacing
    self.set_font("Regular", self.normal_size)
    self.font_mode = 'R'
  def read_footnote(self, footnote_symbol, footnote_text):
    footnote_stream = TextStream(self.footnote_width, self.footnote_align, \
        self.footnote_paragraph_spacing, self.footnote_line_spacing)
    footnote_stream.set_font("Regular", self.footnote_size)
    footnote_stream.add_word(footnote_symbol)
    footnote_stream.set_font("Italic", self.footnote_size)
    footnote_stream.read_words(footnote_text)
    content = footnote_stream.to_content()
    content += "glue {}\n".format(self.footnote_paragraph_spacing)
    content = "flow footnote\n" + content + "flow normal\n"
    self.insert_content(content)
  def read_line(self, line):
    if line[0] == '^':
      parts = line[1:].split(maxsplit = 1)
      if len(parts) < 2:
        return
      footnote_symbol, footnote_text = parts
      footnote_symbol = "{}".format(footnote_symbol)
      self.add_word(footnote_symbol)
      self.read_footnote(footnote_symbol, footnote_text)
      return
    if line[0] == '#':
      line = line[1:]
      level = 1
      while line[0] == '#':
        line = line[1:]
        level += 1
      size = None
      if level > 2:
        level = 2
      size = int(self.normal_size * 1.62 ** (3 - level))
      self.end_paragraph()
      self.set_font("Regular", size)
      self.read_words(line)
      self.set_font("Regular", self.normal_size)
      self.end_paragraph()
      self.font_mode = 'R'
      return
    words = line.split()
    for word in words:
      if word[0] == '*' and self.font_mode == 'R':
        self.set_font("Bold", self.normal_size)
        word = word[1:]
        self.font_mode = 'B'
      elif word[0] == '_' and self.font_mode == 'R':
        self.set_font("Italic", self.normal_size)
        word = word[1:]
        self.font_mode = 'I'
      if len(word) == 0:
        continue
      if word[-1] == '*' and self.font_mode == 'B':
        word = word[:-1]
        self.add_word(word)
        self.set_font("Regular", self.normal_size)
        self.font_mode = 'R'
      elif word[-1] == '_' and self.font_mode == 'I':
        word = word[:-1]
        self.add_word(word)
        self.set_font("Regular", self.normal_size)
        self.font_mode = 'R'
      else:
        self.add_word(word)
    if len(words) == 0:
      self.end_paragraph()

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-w", "--normal_width", type=int, required=True)
arg_parser.add_argument("-W", "--footnote_width", type=int)
arg_parser.add_argument("-s", "--normal_size", type=int, default=12)
arg_parser.add_argument("-S", "--footnote_size", type=int, default=12)
arg_parser.add_argument("-a", "--normal_align", default='l')
arg_parser.add_argument("-A", "--footnote_align", default='l')
arg_parser.add_argument("-l", "--normal_line_spacing", default=0)
arg_parser.add_argument("-L", "--footnote_line_spacing", default=0)
arg_parser.add_argument("-p", "--normal_paragraph_spacing")
arg_parser.add_argument("-P", "--footnote_paragraph_spacing")
args = arg_parser.parse_args()
if args.footnote_width == None:
  args.footnote_width = args.normal_width
if args.normal_paragraph_spacing == None:
  args.normal_paragraph_spacing = args.normal_size
if args.footnote_paragraph_spacing == None:
  args.footnote_paragraph_spacing = args.footnote_size
if not args.normal_align in ('l', 'r', 'c', 'j'):
  warn("Invalid normal align mode.")
  exit(1)
if not args.normal_align in ('l', 'r', 'c', 'j'):
  warn("Invalid footnote align mode.")
  exit(1)

main_stream = MainStream(args.normal_width, args.footnote_width, \
    args.normal_size, args.footnote_size, args.normal_align, \
    args.footnote_align, args.normal_paragraph_spacing, \
    args.normal_line_spacing, args.footnote_paragraph_spacing, \
    args.footnote_line_spacing)
for line in sys.stdin:
  main_stream.read_line(line)
print(main_stream.to_content(), end='')
