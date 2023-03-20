#!/bin/awk -f

BEGIN {
  print "START PAGE"
  page_height = 842
  y = page_height
}
$1 != "#" {
  print $0
  next
}
$2 == "vertical_content" {
  y -= $3
  if (y < 0) {
    y = page_height - $3
    print "END"
    print "START PAGE"
  }
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
