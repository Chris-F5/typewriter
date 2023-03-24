#!/bin/python3

import sys

after_word = False

print("FONT Regular 12")

for line in sys.stdin:
    words = line.split()
    for word in words:
        word = word.replace('"', '\\"')
        if after_word:
            print('OPTBREAK " " ""')
        print('STRING "{}"'.format(word))
        after_word = True
    if len(words) == 0:
        print("BREAK")
        after_word = False
