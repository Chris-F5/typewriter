#!/bin/sh

echo "FONT Regular 12"
sed 's/^ *// ; s/ *$// ; s/  */\n/g ; s/"/\\"/g' \
  | sed '/^./s/^/STRING "/ ; /^./s/$/"\nOPTBREAK " " ""/ ; /^$/s/^/BREAK/'
