#!/bin/sh

#echo 'TEXT /F1 12 Tf'
#sed 's/\\/\\\\/g ; s/(/\\(/g ; s/)/\\)/g' \
#  | awk '{print "MOVE 0 -12" ; print "TEXT (" $0 ") Tj"}'

echo "#SIZE 12"
sed 's/ /\n/g' | sed '/^./s/^/^/ ; /^./s/$/\n\/ %/ ; /^$/s/^/;/'

