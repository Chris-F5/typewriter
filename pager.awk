#!/bin/awk -f

BEGIN { y = 842 }

!g && $1 == "graphic" {
  g = 1
  y -= $3
  printf "GOTO 0 %d\n", y
  next
}
g && $1 == "endgraphic" {
  g = 0
  next
}
g { print $0 }

END { print "PAGE" }
