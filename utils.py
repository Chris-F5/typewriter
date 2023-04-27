import sys, subprocess, re

# utils.py
# Provides utility functions for python scripts part of the typesetting
# pipeline.

# This function is used to invoke the _line_break_ binary. _text_ is passed to
# the _line_break_ program's standard input and this function returns the
# output when its finished. We assume the _line_break_ binary is in the users
# PATH environment variable.
def line_break(text, width, align):
  # Invoke the _line_break_ program with command line arguments.
  process = subprocess.Popen(["line_break", "-" + align, "-w", str(width)],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE)
  # Encode the text, write it to the process's standard input stream.
  text = bytes(text, "ascii")
  process.stdin.write(text)
  # Wait for the program to exit and get the standard output data.
  output, error = process.communicate()
  # Decode and return the standard output.
  return output.decode("ascii")

# Print a string to standard error stream.
def warn(msg):
  print(msg, file=sys.stderr)

# Prepare a string to be put in a quoted record field.
# The backslash is used to escape characters in the field.
# Backslash literals need to be escaped with another backslash.
# A quotation mark is used to end the field. A quotation mark literal must be
# escaped with a backslash.
# Newlines are not allowed.
def strip_string(string):
  return string.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '')

# Parse the next record that appears in _file_.
def parse_record(file):
  fields = []
  # Try to parse a record on each line until a record is found.
  for line in file:
    fields = re.findall(r'[^"\s]\S*|".*?[^\\]"', line)
    if len(fields):
      break
  # If the end of the file was reached without finding a record: return None.
  if len(fields) == 0:
    return None
  # Remove quotes from quoted fields; remove backslash from quotation mark
  # literal.
  for i in range(len(fields)):
    if fields[i][0] == '"':
      fields[i] = fields[i][1:-1] # Remove first and last character (").
    fields[i] = fields[i].replace('\\"', '"')
  return fields
