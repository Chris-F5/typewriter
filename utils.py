import sys, subprocess, re

def line_break(text, width, align):
  # Invoke the _line_break_ program and get the output.
  process = subprocess.Popen(["line_break", "-" + align, "-w", str(width)],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE)
  text = bytes(text, "ascii")
  process.stdin.write(text)
  output, error = process.communicate()
  return output.decode("ascii")

def warn(msg):
  print(msg, file=sys.stderr)

def strip_string(string):
  # Strip a string that is to be written to a quoted record field.
  return string.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '')

def parse_record(file):
  fields = []
  for line in file:
    fields = re.findall(r'[^"\s]\S*|".*?[^\\]"', line)
    if len(fields):
      break
  if len(fields) == 0:
    return None
  # Remove quotes from quoted fields.
  for i in range(len(fields)):
    if fields[i][0] == '"':
      fields[i] = fields[i][1:-1]
    fields[i] = fields[i].replace('\\"', '"')
  return fields
