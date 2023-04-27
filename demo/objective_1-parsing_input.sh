#!/bin/sh
cd $(dirname $0)
PATH=$PATH:..

(markup_text.py -w 390 | pager.py | tw) << END_CONTENT

# Parsing Input

1.1 Escape sequence identifies *bold text*

1.2 Escape sequence identifies _italic text_

## 1.3 Escape sequence identifies header text of multiple sizes

1.4 Escape sequence identifies footnote
^* The footnote is here.

END_CONTENT
