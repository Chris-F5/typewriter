#!/bin/sh
cd $(dirname $0)
PATH=$PATH:..

(sh | pager.py -H "3.7 header" -n -c .contents) > .content_pages << END_CONTENT

(markup_text.py -w 390) << END_TEXT
# 3.1 Lines are fitted onto pages.
3.5 Page numbers appear at the bottom of each page.

## 3.9 You are viewing this in a PDF file.

3.10; 3.11: the fonts and images are embedded into this PDF file.

<-- 3.8 margins can be controlled.

Lorem* ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
incididunt ut labore et dolore magna aliqua. Cursus sit amet dictum sit amet
justo donec enim diam. Amet luctus venenatis lectus magna fringilla. Sit amet
purus gravida quis. Mollis aliquam ut porttitor leo a diam sollicitudin tempor.

3.3 footnote appears on the same page it's referenced
^* 3.2 footnote is at bottom of page.
.

Posuere ac ut consequat semper viverra. Magna fringilla urna porttitor rhoncus
dolor purus non. Faucibus pulvinar elementum integer enim neque volutpat ac.
Nunc mi ipsum faucibus vitae aliquet nec ullamcorper. Felis bibendum ut
tristique et egestas quis. Habitasse platea dictumst quisque sagittis. Non enim
praesent elementum facilisis leo vel fringilla est ullamcorper. Rhoncus mattis
rhoncus urna neque viverra justo nec ultrices dui. Cursus vitae congue mauris
rhoncus aenean. Urna nec tincidunt praesent semper feugiat nibh sed pulvinar.

3.4 Images can be inserted into the content
END_TEXT

echo "mark image"
echo "box 105"
echo "START GRAPHIC"
echo "IMAGE 100 100 peppers.jpg"
echo "END"

(markup_text.py -w 390) << END_TEXT
Euismod elementum nisi quis eleifend quam. Felis imperdiet proin fermentum leo.
Id interdum velit laoreet id donec ultrices tincidunt arcu non. At imperdiet
dui accumsan sit amet nulla. Arcu cursus euismod quis viverra nibh cras
pulvinar mattis nunc. Turpis tincidunt id aliquet risus feugiat. Aliquet
sagittis id consectetur purus ut faucibus pulvinar elementum. Risus commodo
viverra maecenas accumsan. Consectetur purus ut faucibus pulvinar elementum
integer enim neque. Pharetra diam sit amet nisl suscipit adipiscing. Lacus
laoreet non curabitur gravida arcu. Sit amet tellus cras adipiscing enim eu.
Molestie ac feugiat sed lectus vestibulum mattis ullamcorper velit. Fames ac
turpis egestas sed tempus. Elementum nibh tellus molestie nunc non blandit.
Suscipit adipiscing bibendum est ultricies integer. Pellentesque habitant morbi
tristique senectus et netus.

Libero volutpat sed cras ornare arcu dui vivamus arcu felis. Ut faucibus
pulvinar elementum integer enim neque volutpat ac. Nec ullamcorper sit amet
risus nullam eget. A iaculis at erat pellentesque. Nascetur ridiculus mus
mauris vitae ultricies leo integer. Vitae ultricies leo integer malesuada nunc
vel risus. Aliquam malesuada bibendum arcu vitae elementum curabitur. Rhoncus
est pellentesque elit ullamcorper dignissim cras. Nisi est sit amet facilisis
magna etiam tempor orci eu. Mauris pharetra et ultrices neque ornare aenean. Ac
tincidunt vitae semper quis. Gravida quis blandit turpis cursus in hac. Egestas
congue quisque egestas diam in arcu. Id aliquet risus feugiat in. Dui nunc
mattis enim ut tellus elementum sagittis. Id interdum velit laoreet id donec
ultrices tincidunt arcu. Neque viverra justo nec ultrices dui sapien eget mi.
Sit amet purus gravida quis blandit turpis cursus. Proin nibh nisl condimentum
id.

Sit amet nisl suscipit adipiscing bibendum est. Posuere urna nec tincidunt
praesent semper feugiat. At erat pellentesque adipiscing commodo elit.
Fermentum dui faucibus in ornare quam viverra. Ipsum nunc aliquet bibendum enim
facilisis gravida neque. Odio ut enim blandit volutpat maecenas volutpat
blandit aliquam. Aliquet eget sit amet tellus cras adipiscing enim eu turpis.
Id consectetur purus ut faucibus pulvinar elementum integer. Eu mi bibendum
neque egestas congue quisque egestas diam in. Turpis nunc eget lorem dolor sed
viverra. Elit duis tristique sollicitudin nibh sit amet commodo. Sed nisi lacus
sed viverra. Eget magna fermentum iaculis eu non diam. Gravida in fermentum et
sollicitudin ac. Facilisi nullam vehicula ipsum a. Malesuada proin libero nunc
consequat interdum varius sit amet mattis.
END_TEXT
echo 'mark "end of content"'
END_CONTENT

(sh | tw) << END_DOCUMENT
(sh | pager.py) << END_CONTENTS_PAGE
markup_text.py -w 390 << END_TEXT
# 3.6 Contents Page
END_TEXT
echo "glue 20"
contents.py < .contents
END_CONTENTS_PAGE
cat .content_pages
END_DOCUMENT
