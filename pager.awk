#!/bin/awk -f

BEGIN {
  print "START PAGE"
  y = 842
}
$1 != "#" {
  print $0
  next
}
$2 == "vertical_content" {
  y -= $3
  printf "MOVE 0 %d\n", y
  next
}
$2 == "glue" {
  y -= $3
  next
}
END {
  print "END"
}
