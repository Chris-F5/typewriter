import sys, subprocess, re

def line_break(text, width, align):
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
  return string.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '')

def parse_record(file):
  fields = []
  for line in file:
    fields = re.findall(r'[^"\s]\S*|".*?[^\\]"', line)
    if len(fields):
      break
  if len(fields) == 0:
    return None
  for i in range(len(fields)):
    if fields[i][0] == '"':
      fields[i] = fields[i][1:-1]
    fields[i] = fields[i].replace('\\"', '"')
  return fields
