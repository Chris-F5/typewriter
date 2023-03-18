#!/bin/sh

#echo 'TEXT /F1 12 Tf'
#sed 's/\\/\\\\/g ; s/(/\\(/g ; s/)/\\)/g' \
#  | awk '{print "MOVE 0 -12" ; print "TEXT (" $0 ") Tj"}'

echo "FONT Regular 12"
sed 's/^ *// ; s/  */\n/g ; s/"/\\"/g' \
  | sed '/^./s/^/STRING "/ ; /^./s/$/"\nOPTBREAK " " ""/ ; /^$/s/^/BREAK/'
