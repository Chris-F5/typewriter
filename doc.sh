#!/bin/sh

(sh | ./pager.py -n -c contents) > content_pages << END_CONTENT

echo 'mark "document start"'

(./markup_text.py -w 390 -A r -S 20 -P 50 -L 10) << END_PARAGRAPH
Hello _world,_ this is a footnote
^1 this is the content of the first footnote which spans multiple lines
and some more text
^2 this is a second footnote
here which continues until a line break occurs.

some text here
# Header 1
## Header 2
### Header 3
#### Header 4

A new *paragraph begins here.
END_PARAGRAPH

echo 'mark "before graphic"'

echo "glue 10"
echo "box 100"
echo "START GRAPHIC"
echo "IMAGE 100 100 peppers.jpg"
echo "END"
echo "glue 10"

echo 'mark "after graphic"'

(./markup_text.py -w 390 -s 10 -a j -p 20 -l 2) << END_PARAGRAPH
Lorem* ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Cursus sit amet dictum sit amet justo donec enim diam. Amet luctus venenatis lectus magna fringilla. Sit amet purus gravida quis. Mollis aliquam ut porttitor leo a diam sollicitudin tempor. Leo a diam sollicitudin tempor. Vitae ultricies leo integer malesuada nunc vel risus commodo viverra. Mollis aliquam ut porttitor leo. Nunc pulvinar sapien et ligula ullamcorper malesuada proin libero nunc. Eu augue ut lectus arcu bibendum at varius vel. Eget gravida cum sociis natoque penatibus et magnis dis. In tellus integer feugiat scelerisque varius morbi. Ullamcorper sit amet risus nullam eget felis. Tortor dignissim convallis aenean et tortor.

Posuere ac ut consequat semper viverra. Magna fringilla urna porttitor rhoncus dolor purus non. Faucibus pulvinar elementum integer enim neque volutpat ac. Nunc mi ipsum faucibus vitae aliquet nec ullamcorper. Felis bibendum ut tristique et egestas quis. Habitasse platea dictumst quisque sagittis. Non enim praesent elementum facilisis leo vel fringilla est ullamcorper. Rhoncus mattis rhoncus urna neque viverra justo nec ultrices dui. Cursus vitae congue mauris rhoncus aenean. Urna nec tincidunt praesent semper feugiat nibh sed pulvinar. Faucibus ornare suspendisse sed nisi lacus sed viverra tellus in. In hac habitasse platea dictumst quisque sagittis purus. Tellus integer feugiat scelerisque varius. Tellus integer feugiat scelerisque varius morbi. Vitae ultricies leo integer malesuada nunc vel risus commodo viverra. Commodo quis imperdiet massa tincidunt nunc pulvinar sapien et. Enim facilisis gravida neque convallis a cras semper auctor. Aenean vel elit scelerisque mauris pellentesque pulvinar pellentesque habitant morbi. Orci eu lobortis elementum nibh tellus. Cras adipiscing enim eu turpis egestas.

Euismod elementum nisi quis eleifend quam. Felis imperdiet proin fermentum leo. Id interdum velit laoreet id donec ultrices tincidunt arcu non. At imperdiet dui accumsan sit amet nulla. Arcu cursus euismod quis viverra nibh cras pulvinar mattis nunc. Turpis tincidunt id aliquet risus feugiat. Aliquet sagittis id consectetur purus ut faucibus pulvinar elementum. Risus commodo viverra maecenas accumsan. Consectetur purus ut faucibus pulvinar elementum integer enim neque. Pharetra diam sit amet nisl suscipit adipiscing. Lacus laoreet non curabitur gravida arcu. Sit amet tellus cras adipiscing enim eu. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper velit. Fames ac turpis egestas sed tempus. Elementum nibh tellus molestie nunc non blandit. Suscipit adipiscing bibendum est ultricies integer. Pellentesque habitant morbi tristique senectus et netus.

Libero volutpat sed cras ornare arcu dui vivamus arcu felis. Ut faucibus pulvinar elementum integer enim neque volutpat ac. Nec ullamcorper sit amet risus nullam eget. A iaculis at erat pellentesque. Nascetur ridiculus mus mauris vitae ultricies leo integer. Vitae ultricies leo integer malesuada nunc vel risus. Aliquam malesuada bibendum arcu vitae elementum curabitur. Rhoncus est pellentesque elit ullamcorper dignissim cras. Nisi est sit amet facilisis magna etiam tempor orci eu. Mauris pharetra et ultrices neque ornare aenean. Ac tincidunt vitae semper quis. Gravida quis blandit turpis cursus in hac. Egestas congue quisque egestas diam in arcu. Id aliquet risus feugiat in. Dui nunc mattis enim ut tellus elementum sagittis. Id interdum velit laoreet id donec ultrices tincidunt arcu. Neque viverra justo nec ultrices dui sapien eget mi. Sit amet purus gravida quis blandit turpis cursus. Proin nibh nisl condimentum id.

Sit amet nisl suscipit adipiscing bibendum est. Posuere urna nec tincidunt praesent semper feugiat. At erat pellentesque adipiscing commodo elit. Fermentum dui faucibus in ornare quam viverra. Ipsum nunc aliquet bibendum enim facilisis gravida neque. Odio ut enim blandit volutpat maecenas volutpat blandit aliquam. Aliquet eget sit amet tellus cras adipiscing enim eu turpis. Id consectetur purus ut faucibus pulvinar elementum integer. Eu mi bibendum neque egestas congue quisque egestas diam in. Turpis nunc eget lorem dolor sed viverra. Elit duis tristique sollicitudin nibh sit amet commodo. Sed nisi lacus sed viverra. Eget magna fermentum iaculis eu non diam. Gravida in fermentum et sollicitudin ac. Facilisi nullam vehicula ipsum a. Malesuada proin libero nunc consequat interdum varius sit amet mattis.

More content that spills to the next page

END_PARAGRAPH

echo 'mark "end content"'

END_CONTENT

(sh | ./tw) << END_DOCUMENT
(./contents.py | ./pager.py) < contents
cat content_pages
cat << END_PAGE
START PAGE
MOVE 100 420
START TEXT
FONT Regular 12
STRING "END OF DOCUMENT"
END
END
END_PAGE
END_DOCUMENT
