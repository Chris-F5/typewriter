#!/bin/awk -f

BEGIN {
  print "START PAGE"
  y = 842
  content_depth = 0
}
$1 != "#" {
  print $0
  next
}
$2 == "vertical_content" {
  content_depth = 1
  y -= $3
  printf "MOVE 0 %d\n", y
  next
}
END {
  print "END"
}
