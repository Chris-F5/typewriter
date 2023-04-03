#!/bin/python3

import sys, subprocess

footnotes = []

def line_break(text, width):
  process = subprocess.Popen(["./line_break", "-j", "-l", str(width)],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE)
  text = bytes(text, "ascii")
  process.stdin.write(text)
  output, error = process.communicate()
  return output.decode("ascii")

class TextStream:
  def __init__(self, width, base_size):
    self.width = width
    self.base_size = base_size
    self.in_paragraph = False
    self.in_string = False
    self.text = ""
    self.insertions = []
  def add_string(self, string):
    if not self.in_string:
      self.text += 'STRING "'
      self.in_string = True
    string = string.replace('"', '\\"')
    string = string.replace('\n', '')
    self.text += string
  def close_string(self):
    if self.in_string:
      self.text += '"\n'
      self.in_string = False
  def set_font(self, font_name, font_size):
    self.close_string()
    self.text += "FONT {} {}\n".format(font_name, font_size)
  def add_word(self, word):
    self.close_string()
    if self.in_paragraph:
      self.text += 'OPTBREAK " " ""\n'
    self.in_paragraph = True
    self.add_string(word)
  def end_paragraph(self):
    if self.in_paragraph:
      self.close_string()
      self.text += "BREAK\n"
      self.in_paragraph = False
  def insert_content(self, insertion):
    self.close_string()
    mark = len(self.insertions)
    self.insertions.append(insertion)
    self.text += "MARK {}\n".format(mark)
  def to_content(self):
    self.close_string()
    content = line_break(self.text, self.width)
    for i in range(len(self.insertions)):
      content = content.replace('\n^' + str(i), '\n' + self.insertions[i])
    return content
  def read_words(self, line):
    words = line.split()
    for word in words:
      self.add_word(word)

class MainStream(TextStream):
  def __init__(self, normal_width, footnote_width, base_size):
    super().__init__(normal_width, base_size)
    self.footnote_width = footnote_width
    self.set_font("Regular", self.base_size)
    self.font_mode = 'R'
  def read_footnote(self, footnote_symbol, footnote_text):
    footnote_stream = TextStream(self.footnote_width, self.base_size)
    footnote_stream.set_font("Regular", footnote_stream.base_size)
    footnote_stream.add_word(footnote_symbol)
    footnote_stream.set_font("Italic", footnote_stream.base_size)
    footnote_stream.read_words(footnote_text)
    content = footnote_stream.to_content()
    content = "flow footnote\n" + content + "flow normal\n"
    self.insert_content(content)
  def read_line(self, line):
    if line[0] == '^':
      parts = line[1:].split(maxsplit = 1)
      if len(parts) < 2:
        return
      footnote_symbol, footnote_text = parts
      footnote_symbol = "[{}]".format(footnote_symbol)
      self.add_word(footnote_symbol)
      self.read_footnote(footnote_symbol, footnote_text)
      return
    if line[0] == '#':
      line = line[1:]
      level = 1
      while line[0] == '#':
        line = line[1:]
        level += 1
      if level > 4:
        level = 4
      size = self.base_size + 24 // level
      self.end_paragraph()
      self.set_font("Regular", size)
      self.read_words(line)
      self.set_font("Regular", self.base_size)
      self.end_paragraph()
      self.font_mode = 'R'
      return
    words = line.split()
    for word in words:
      if word[0] == '*' and self.font_mode == 'R':
        self.set_font("Bold", self.base_size)
        word = word[1:]
        self.font_mode = 'B'
      if word[0] == '_' and self.font_mode == 'R':
        self.set_font("Italic", self.base_size)
        word = word[1:]
        self.font_mode = 'I'
      if word[-1] == '*' and self.font_mode == 'B':
        word = word[:-1]
        self.add_word(word)
        self.set_font("Regular", self.base_size)
        self.font_mode = 'R'
        continue
      if word[-1] == '_' and self.font_mode == 'I':
        word = word[:-1]
        self.add_word(word)
        self.set_font("Regular", self.base_size)
        self.font_mode = 'R'
        continue
      self.add_word(word)
    if len(words) == 0:
      self.end_paragraph()

main_stream = MainStream(475, 475, 12)
for line in sys.stdin:
  main_stream.read_line(line)
print(main_stream.to_content(), end='')
