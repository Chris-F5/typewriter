#!/bin/sh


markup_text.py -w 390 << END_TEXT
### Objectives Met Demonstration

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

cat << END_FONT_DEMO
box 12
START TEXT
FONT Demo 12
STRING "Other fonts can be imported"
FONT Bold 12
STRING " and mixed "
FONT Italic 12
STRING "with eachother!"
END
END_FONT_DEMO
