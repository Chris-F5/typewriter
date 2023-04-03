#!/bin/python3

import sys, subprocess

footnotes = []

def line_break(text):
  process = subprocess.Popen(['./line_break', '-j', '-l', '475'],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE)
  text = bytes(text, "ascii")
  process.stdin.write(text)
  output, error = process.communicate()
  return output.decode("ascii")

def format_words(text, first_word = True):
  string = ''
  words = text.split()
  for word in words:
    if not first_word:
      string += 'OPTBREAK " " ""\n'
    first_word = False
    string += 'STRING "{}"\n'.format(word.replace('"', '\\"'))
  return string

def process_footnote(string, symbol):
  text = 'FONT Regular 12\n'
  text += 'STRING "{}"\n'.format(symbol)
  text += 'FONT Italic 12\n'
  text += format_words(string)
  return line_break(text)

footnotes = []
text = 'FONT Regular 12\n'
paragraph_start = True
for line in sys.stdin:
  if paragraph_start == False and line == '\n':
    text += 'BREAK\n'
    paragraph_start = True
    continue
  if line[0] == '^':
    parts = line[1:].split(maxsplit = 1)
    if len(parts) < 2:
      continue
    footnote_symbol, footnote_text = parts
    footnote_symbol = '[{}]'.format(footnote_symbol.replace('"', '\\"'))
    text += 'STRING "{}"\n'.format(footnote_symbol)
    text += 'MARK {}\n'.format(len(footnotes))
    footnotes.append(process_footnote(footnote_text, footnote_symbol))
  else:
    text += format_words(line, paragraph_start)
  paragraph_start = False
content = line_break(text)
for i in range(len(footnotes)):
  footnote = 'flow footnote\n' + footnotes[i] + 'flow normal'
  content = content.replace('\n^' + str(i), '\n' + footnote)
print(content, end='')
