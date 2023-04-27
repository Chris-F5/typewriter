#!/bin/python3

# markup_text.py
# Read markup text from standard input.
# Typeset this text and write _content_ to standard output.

import sys, argparse
from utils import *

# A _TextStream_ builds a _text_specification_.
# By invoking _line_break_ this can be converted into _content_.
class TextStream:
  def __init__(self, width, align, paragraph_spacing, line_spacing):
    self.width = width
    # _align_ is the align mode for this text. 'l', 'r', 'c' or 'j'.
    self.align = align
    self.paragraph_spacing = paragraph_spacing
    self.line_spacing = line_spacing
    self.in_paragraph = False
    self.in_string = False
    # _text_ is the _text_specification_ string.
    self.text = ""
    # An insertion is some _content_ to be inserted after the line of text that
    # the insertion appears on. For example, a footnote is a type of insertion.
    self.insertions = []
  # Add a string to the _text_specification_.
  def add_string(self, string):
    # If not already in a string then start one.
    if not self.in_string:
      self.text += 'STRING "'
      self.in_string = True
    # Add the stripped text to the _text_specification_.
    self.text += strip_string(string)
  def close_string(self):
    # If _text_specification_ is in a string then close it and get ready for
    # the next _text_specification_ command.
    if self.in_string:
      self.text += '"\n'
      self.in_string = False
  # Sets the font to be used for subsequent strings.
  def set_font(self, font_name, font_size):
    # Close the string if we are in one.
    self.close_string()
    # Add the FONT command to the _text_specification_.
    self.text += "FONT {} {}\n".format(font_name, font_size)
  # Add a word to the _text_specification_.
  def add_word(self, word):
    # If empty do nothing.
    if len(word) == 0:
      return
    # Close a string if one is open.
    self.close_string()
    # If this is not the first word of the paragraph add an optional break
    # that inserts a space when no break occurs.
    if self.in_paragraph:
      self.text += 'OPTBREAK " " "" {}\n'.format(self.line_spacing)
    # We are now in a paragraph
    self.in_paragraph = True
    # The word itself is added.
    self.add_string(word)
  def end_paragraph(self):
    if self.in_paragraph:
      self.close_string()
      # Add a forced line break here.
      self.text += "BREAK {}\n".format(self.paragraph_spacing)
      self.in_paragraph = False
  # _insertion_ is _content_ that must appear on this line.
  def insert_content(self, insertion):
    self.close_string()
    # Get the unique identifier for this mark.
    mark = len(self.insertions)
    # Add the insertion to the list of insertions.
    self.insertions.append(insertion)
    # Add the MARK into the text_specification.
    self.text += "MARK {}\n".format(mark)
  # Convert this _text_specification_ to _content_ by finding optimal line
  # breaks.
  def to_content(self):
    self.close_string()
    # Invoke the line_break binary with _self.text_ input.
    content = line_break(self.text, self.width, self.align)
    # Insert the insertion content after line breaking.
    for i in range(len(self.insertions)):
      content = content.replace('\n^' + str(i), '\n' + self.insertions[i])
    return content
  # Read whitespace separated words from line into the text stream.
  def read_words(self, line):
    # Split the line into words by whitespace.
    words = line.split()
    # Add each word individually.
    for word in words:
      self.add_word(word)

# The _MainStream_ INHERITS from text _TextStream_.
# It is used for passing all markup text excluding the content of footnotes.
# It is able to parse bold text, footnotes and other markup features.
class MainStream(TextStream):
  def __init__(self, normal_width, footnote_width, normal_size, footnote_size, \
      normal_align, footnote_align, normal_paragraph_spacing, \
      normal_line_spacing, footnote_paragraph_spacing, footnote_line_spacing):
    # Initialise the superclass.
    super().__init__(normal_width, normal_align, normal_paragraph_spacing, \
        normal_line_spacing)
    self.footnote_width = footnote_width
    self.normal_size = normal_size
    self.footnote_size = footnote_size
    self.footnote_align = footnote_align
    self.footnote_line_spacing = footnote_line_spacing
    self.footnote_paragraph_spacing = footnote_paragraph_spacing
    self.set_font("Regular", self.normal_size)
    # Remember the last font set so that if more Regular text is encountered
    # there is no need to unnecessarily set the font again.
    self.font_mode = 'R'
  # Add a footnote to the stream.
  def read_footnote(self, footnote_symbol, footnote_text):
    # Make a new stream for this footnotes content.
    footnote_stream = TextStream(self.footnote_width, self.footnote_align, \
        self.footnote_paragraph_spacing, self.footnote_line_spacing)
    # Write footnote symbol and text to the new stream.
    footnote_stream.set_font("Regular", self.footnote_size)
    footnote_stream.add_word(footnote_symbol)
    footnote_stream.set_font("Italic", self.footnote_size)
    footnote_stream.read_words(footnote_text)
    # Convert the stream to content (line break the footnote).
    content = footnote_stream.to_content()
    # Add some glue to the end so footnotes are separated.
    content += "glue {}\n".format(self.footnote_paragraph_spacing)
    # Set the flow for the footnote content.
    content = "flow footnote\n" + content + "flow normal\n"
    # Insert the footnote content into the MainStream at this point.
    self.insert_content(content)
  # Read a line of markup text and add it to the stream.
  def read_line(self, line):
    # If the line begins with a carrot (^) then it is a footnote.
    if line[0] == '^':
      # The first part of the footnote after the carrot (&) is the footnote
      # symbol. The rest is the footnote text.
      parts = line[1:].split(maxsplit = 1)
      # If there was not a footnote synbol and text after the carrot then
      # ignore this line.
      if len(parts) < 2:
        return
      footnote_symbol, footnote_text = parts
      # Write the footnote symbol to main text and add the footnote itself.
      self.add_word(footnote_symbol)
      self.read_footnote(footnote_symbol, footnote_text)
      # Exit the function.
      return
    # A line starting with one or more hashtags (#) is a header.
    if line[0] == '#':
      # Remove the first hashtag.
      line = line[1:]
      level = 1
      # For each subsequent hashtag, increase the level by 1.
      while line[0] == '#':
        line = line[1:]
        level += 1
      # More than 2 hashtags make no difference.
      if level > 2:
        level = 2
      # Calculate the text size for the header.
      size = int(self.normal_size * 1.62 ** (3 - level))
      # End any open paragraphs.
      self.end_paragraph()
      # Write the header with _size_.
      self.set_font("Regular", size)
      self.read_words(line)
      # Return to normal size and font.
      self.set_font("Regular", self.normal_size)
      # The header is technically a paragraph that needs to be ended.
      self.end_paragraph()
      self.font_mode = 'R'
      # Exit the function.
      return
    # The line is a list of whitespace separated words.
    words = line.split()
    # Loop through each word.
    for word in words:
      # Bold text is enclosed in stars (*), italic in underscores (_).
      # If the word begins with a star (*) or underscore (_) and the font mode
      # is Regular. Then we switch to the new font mode and remove the star or
      # underscore from the start of the word.
      if word[0] == '*' and self.font_mode == 'R':
        self.set_font("Bold", self.normal_size)
        word = word[1:]
        self.font_mode = 'B'
      elif word[0] == '_' and self.font_mode == 'R':
        self.set_font("Italic", self.normal_size)
        word = word[1:]
        self.font_mode = 'I'
      # If the word is empty then go to the next word.
      if len(word) == 0:
        continue
      # If a star (*) or underscore (_) ends bold or italic font mode. Then
      # remove the star or underscore from the end of the word, add the word
      # and return to Regular font mode.
      # Else: if its just a normal word then add it.
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
    # If there were no words on this line then end the paragraph.
    if len(words) == 0:
      self.end_paragraph()

# Parse command line arguments.
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

# Some command line arguments default to values of other arguments.
if args.footnote_width == None:
  args.footnote_width = args.normal_width
if args.normal_paragraph_spacing == None:
  args.normal_paragraph_spacing = args.normal_size
if args.footnote_paragraph_spacing == None:
  args.footnote_paragraph_spacing = args.footnote_size

# Verify align mode command line arguments are valid.
# 'l', 'r', 'c', 'j' are text alignment modes, short for: left, right, centre,
# justified.
if not args.normal_align in ('l', 'r', 'c', 'j'):
  warn("Invalid normal align mode.")
  exit(1)
if not args.normal_align in ('l', 'r', 'c', 'j'):
  warn("Invalid footnote align mode.")
  exit(1)

# Create a MainStream with the command line options.
main_stream = MainStream(args.normal_width, args.footnote_width, \
    args.normal_size, args.footnote_size, args.normal_align, \
    args.footnote_align, args.normal_paragraph_spacing, \
    args.normal_line_spacing, args.footnote_paragraph_spacing, \
    args.footnote_line_spacing)
# For each line in the standard input, parse this line with the MainStream.
for line in sys.stdin:
  main_stream.read_line(line)
# Convert the MainStream to _content_ and write to standard output.
print(main_stream.to_content(), end='')
