#!/bin/sh
cd $(dirname $0)
PATH=$PATH:..

(sh | pager.py | tw) << END_DOCUMENT

(markup_text.py -w 390 -a j) << END_TEXT
2.1 Optimal line breaks are selected.

2.2 Spaces are inserted when no break occurs.

Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
incididunt ut labore et dolore magna aliqua. Cursus sit amet dictum sit amet
justo donec enim diam. Amet luctus venenatis lectus magna fringilla. Sit amet
purus gravida quis. Mollis aliquam ut porttitor leo a diam sollicitudin tempor.
Leo a diam sollicitudin tempor. Vitae ultricies leo integer malesuada nunc vel
risus commodo viverra. Mollis aliquam ut porttitor leo. Nunc pulvinar sapien et
ligula ullamcorper malesuada proin libero nunc. Eu augue ut lectus arcu
bibendum at varius vel. Eget gravida cum sociis natoque penatibus et magnis
dis. In tellus integer feugiat scelerisque varius morbi. Ullamcorper sit amet
risus nullam eget felis. Tortor dignissim convallis aenean et tortor.
END_TEXT

line_break -w 190 << END_TEXT_SPEC
FONT Regular 12
STRING "2.4"
OPTBREAK " " "" 0
FONT Regular 18
STRING "varied"
OPTBREAK " " "" 0
FONT Demo 8
STRING "fonts."
OPTBREAK " " "" 0
FONT Regular 12
STRING "and"
OPTBREAK " " "" 0
STRING "2.3"
OPTBREAK " " "" 0
STRING "hypena"
OPTBREAK "" "-" 0
STRING "tion."
END_TEXT_SPEC

END_DOCUMENT
