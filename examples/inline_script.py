import sys
import subprocess

shell = False
command = ""

def run_shell(command):
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True)
    sys.stderr.write(result.stderr)
    sys.stdout.write(result.stdout)
    if result.returncode != 0:
        exit(1)

def run(payload, mode):
    result = subprocess.run(mode, input=payload, shell=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sys.stderr.write(result.stderr)
    sys.stdout.write(result.stdout)
    if result.returncode != 0:
        exit(1)

if len(sys.argv) == 1:
    FILE=sys.stdin
elif len(sys.argv) == 2:
    FILE=open(sys.argv[1], 'r')
else:
    sys.stderr.write("Too many arguments\n")
    exit(1)

mode = "verbatim"
payload = ""

for line in FILE:
    if len(line) > 0 and line[0] == '#':
        continue
    if mode == "verbatim":
        parts = line.strip().split(" ", 1)
        if parts[0] == "&BEGIN":
            mode = parts[1]
        else:
            sys.stdout.write(line)
    else:
        if line.strip() == "&END":
            run(payload, mode)
            mode = "verbatim"
            payload = ""
        else:
            payload += line
