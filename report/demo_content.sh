#!/bin/sh

markup_text.py -w 390 << END_TEXT
## Objectives Met Demonstration

# Large Header
## Small Header

Hello world, this is a demonstration! *Bold text,* _italic text,_ a line break
occurs here.
Footnotes
^* footnotes are indeed supported.
and images are supported:
END_TEXT

echo "box 110"
echo "START GRAPHIC"
echo "IMAGE 100 100 peppers.jpg"
echo "END"

line_break -w 100 -r << END_FONT_DEMO
FONT Regular 12
STRING "Other" 
OPTBREAK " " "" 0
FONT Demo 12
STRING "fonts"
OPTBREAK " " "" 0
FONT Regular 12
STRING "and"
OPTBREAK " " "" 0
FONT Regular 18
STRING "sizes"
FONT Regular 12
OPTBREAK " " "" 0
STRING "can"
OPTBREAK " " "" 0
STRING "be"
OPTBREAK " " "" 0
STRING "mixed."
END_FONT_DEMO

echo "glue 10"

line_break -w 130 << END_LINE_DEMO
FONT Regular 12
STRING "This"
OPTBREAK " " "" 0
STRING "text"
OPTBREAK " " "" 0
STRING "demonstrates"
OPTBREAK " " "" 0
STRING "line"
OPTBREAK " " "" 0
STRING "breaks"
OPTBREAK " " "" 0
STRING "and"
OPTBREAK " " "" 0
STRING "hyphen"
OPTBREAK "" "-" 0
STRING "ation."
END_LINE_DEMO

cal -y | markup_raw.py -s 6
