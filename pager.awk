#!/bin/awk -f

BEGIN {
  print "START PAGE"
  top_margin = 40
  bot_margin = 40
  left_margin = 60
  page_height = 842
  y = page_height - top_margin
  x = left_margin
}
$1 != "#" {
  print $0
  next
}
$2 == "vertical_content" {
  y -= $3
  if (y < bot_margin) {
    y = page_height - top_margin - $3
    print "END"
    print "START PAGE"
  }
  printf "MOVE %d %d\n", x, y
  next
}
$2 == "glue" {
  y -= $3
  next
}
END {
  print "END"
}
