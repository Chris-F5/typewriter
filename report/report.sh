#!/bin/bash

# Change into directory of this script.
cd $(dirname $0)

# Add project binaries to path.
PATH=$PATH:..

body_width=390
new_page="echo 'glue 10000' ; echo 'opt_break'"
markup_text="(markup_text.py -w $body_width -s 10 -a j -A l ; echo 'glue 10')"
markup_list="(markup_raw.py -s 10 -f Regular ; echo 'glue 10')"
markup_code="(markup_raw.py -o 8 -w 4 -s 9; echo 'glue 10')"
mark="printf 'mark \"%s\"\n'"

cite_groff=1
cite_texbook=2
cite_breaking_paragraphs_into_lines=3
cite_how_browsers_work=4
cite_lets_build_a_browser_engine=5
cite_ttf_spec=6
cite_pdf_spec=7
cite_introduction_to_algorithms=8

(bash | pager.py -n -c .contents -H "Christopher Lang | Typesetting System") > .content_pages << END_CONTENT

section_counter=0
subsection_counter=0
subsubsection_counter=0

function header {
  subsection_counter=0
  section_counter=\$((section_counter+1))
  title="\$section_counter \$1"
  echo "new_page"
  echo "glue 60"
  echo "\$title" | markup_text.py -w $body_width -s 26 -a l | sed 's/^opt_break$//'
  $mark "\$title"
  echo "glue 16"
}

function subheader {
  subsubsection_counter=0
  subsection_counter=\$((subsection_counter+1))
  title="  \$section_counter.\$subsection_counter \$1"
  echo "\$title" | markup_text.py -w $body_width -s 16 -a l | sed 's/^opt_break$//'
  $mark "\$title"
  echo "glue 5"
}

function subsubheader {
  subsubsection_counter=\$((subsubsection_counter+1))
  title=" \$section_counter.\$subsection_counter.\$subsubsection_counter \$1"
  echo "\$title" | markup_text.py -w $body_width -s 14 -a l | sed 's/^opt_break$//'
  echo "glue 4"
}

header "Analysis"

subheader "Problem Statement"
$markup_text << END_TEXT
To create a document preparation system for project reports.
END_TEXT

subheader "Problem Background and Analysis"
$markup_text << END_TEXT
As a student, I often need to create digital, printable documents.
I have experimented with a range of document preparation systems: Microsoft
Word, Markdown, LaTeX, etc.
But I am yet to find a solution that is simultaneously simple, powerful and
easy to use.
This project intends to achieve all three of these goals.

A major part of document preparation is text processing.
This involves converting markup into formatted lines of text.
Markup describes the content and can influence the formatting of the text.
Digital text processing systems typically come in three types
^* Coombs, James H.; Renear, Allen H.; DeRose, Steven J. (November 1987). Markup systems and the future of scholarly text processing. Communications of the ACM 30 http://xml.coverpages.org/coombs.html
: presentational, procedural and descriptive.

Presentational systems provide a GUI "what you see is what you get" (WSYIWYG)
editor to enable the user to modify the documents markup.
Text is typeset and displayed to the user as they write.
This type of editor is often easy to use, but it is difficult to have a wide
range of capabilities within the confines of a GUI/window system [$cite_groff].

Procedural markup consists of a sequence of commands that instruct the software
how to format the text.
By combining a small set of simple commands, complex behaviour can be achieved.
However, as commands are processed sequentially, it is often difficult to have
the formatting of earlier text depend on later content.
Consider a two-column page with footnotes that span the entire page.
The first column is written and formatted with no footnotes and then a
footnote appears on the second column which reduces the height available for
the first column.
Clearly, the first column must be re-formatted to make it's text shorter.
But what if this causes the footnote to appear on the next page - now the first
page's columns are shorter for no reason!
As you can see, procedural markup has difficulty in solving certain typesetting
problems.

Descriptive markup identifies what each part of text IS and not how to typeset
it.
For example, a header may be surrounded in _<header> </header>_ tags.
Then, the text-processing system is responsible for deciding how to format this
header.
This system is more flexible than procedural markup, the footnote problem
described in the last problem can be solved simply by marking a section of text
as a footnote and relying on the typesetting software to insert it in the right
page.
However, 'tags' cant be combined as effectively as Procedural markup commands,
this means descriptive markup languages can quickly become very complicated
with a huge number of 'tags' that the user must know (think HTML).

Apart from text-processing, document preparation systems must be able to insert
graphics into page content and output to a printable file format.
PDF and PostScript are the most common of such formats.
PostScript is the older of the two formats, it is a text based format which
may make it easier to generate.
PDF is more widely used, has a richer set of features and its standard is
better documented.
END_TEXT

subheader "Intended End-User"
$markup_text << END_TEXT
Users of Unix-like operating systems who need to generate PDF documents.
END_TEXT

subheader "Third Party"
$markup_text << END_TEXT
Anthony Ceponis uses a Linux based operating system and has recently finished
writing a computer science NEA using 'google docs' - a WYSIWYG editor.
The following is an extract of the transcript of a conversation I had with
Anthony in order to better understand the requirements of the end-user.

*Christopher:* Whats your general opinion of writing in a WSYIWYG editor
compared to writing documents in a markup language like HTML?

*Anthony:* With a GUI editor, everything is much easier and
convenient and quicker (unless your WPM is out of this world) and more
accessible to the average person.

*Christopher:* Did you include a copy of your source code in the NEA report?

*Anthony* I was writing the NEA documentation using google docs and I had to
endure the painful process of taking a screen shot of over 10,000 lines of code
in small chunks and copy pasting them into my report as part of my technical
solution.

*Christopher:* Would you benefit from a program that was capable of
automatically inserting the source code into the PDF document?

*Anthony* I would do unspeakable thinks to have access to a program/feature
that would achieve what you just described.
END_TEXT

subheader "Research and Modelling"
$markup_text << END_TEXT
To embark my research, I examined some existing solutions:

*groff.* First released in 1990, groff is GNU's replacement for
troff.
Like troff, groff makes use of Unix pipes to process documents in several
modular stages.
Groffs compatibility with Unix systems is, in my eyes, its greatest strength
because, unlike graphical editors, Groff can be invoked programmatically
to take input from existing files or streams.
In addition, the groff input file syntax and syntax of many groff preprocessors
is designed to be simple to parse and generate through Unix streams.
This makes it easy to write programs to process or modify the document before
it is typeset.
The groff manual [$cite_groff] was a valuable resource in understanding how
procedural markup languages can solve difficult typesetting problems in a
single pass.
The following diagram shows how Unix pipes are used to modularize the
typesetting process. Each box is a binary program and each arrow is a Unix
pipe.
END_TEXT

echo "box 117"
echo "START GRAPHIC"
echo "IMAGE 390 117 groff.jpg"
echo "END"

$markup_text << END_TEXT
*TeX + LaTeX.* TeX is a typesetting system first released in 1978.
I used a derivative of TeX, LaTeX, to write a number of documents for a school
project.
My frustration with LaTeX's unreadable syntax and confusing extension system
was the initial impetus to make an alternative.
The TeXbook [$cite_texbook] discusses a model of untypeset content consisting
of an ordered list of _gizmos_ each representing an atom of content.
This model was an promising starting point in my thoughts about how to best
define document content before it is typeset.
Donald E Knuth's paper 'Breaking Paragraphs into Lines'
[$cite_breaking_paragraphs_into_lines] describes the line breaking algorithm
used by TeX and compares it to the more primitive 'first fit' method.
The following image shows how _box, glue,_ and _penalty_ gizmos are used to
model the text "Hello World!".
END_TEXT

echo "box 77"
echo "START GRAPHIC"
echo "IMAGE 300 77 gizmos.jpg"
echo "END"

$markup_text << END_TEXT
*HTML + CSS.* Although browsers do not need to split webpage content into
pages, text and graphics must still be arranged on the screen depending on
resolution.
I read an online article about how browsers work [$cite_how_browsers_work] and
a blog about building a browser engine [$cite_lets_build_a_browser_engine] to
understand this layout process.
The hierarchical DOM provides an alternative document content model to TeX's
linear _gizmos._
The following diagram shows how CSS "block" and "inline" boxes are placed on a
page.
END_TEXT

echo "box 260"
echo "START GRAPHIC"
echo "IMAGE 390 260 css.jpg"
echo "END"
echo "glue 5"

$markup_text << END_TEXT
In order to better understand the technical requirements of the project, I
conducted some more specific research into the implantation details.

*TrueType Reference Manual.* [$cite_ttf_spec] This reference manual defines
the binary file format of 'true type' font files.
Parsing a font file to extract glyph widths and other such data is an essential
step in the typesetting process.

*ISO 32000-1 Portable Document Format 1.7.* [$cite_pdf_spec] This document is
the technical specification for the PDF 1.7 file format.
The project intends to generate PDF files so understanding the file format is
necessary.

*Single-source shortest path in DAGs.* [$cite_introduction_to_algorithms]
This single-source shortest path in directed acyclic graphs algorithm can be
used to solve the line breaking problem as modeled in Donald E. Knuth's
'Breaking paragraphs into lines' [$cite_breaking_paragraphs_into_lines] in
O(V+E) time.
In practice, not all feasible lines are known before the algorithm starts, so
the algorithm will need to be modified to search for the feasible edges as it
progresses through the text.
END_TEXT

subheader "Objectives"
$markup_text << END_TEXT
I have decided to make document preparation software which uses a
text-processing system of the 'descriptive' type (though some procedural
features may be supported).
After considering my online secondary research and discussion with Anthony, I
have produced the following set of specific and measurable objectives.
END_TEXT
$markup_code << END_OBJECTIVES
Typeset PDF Document
  1. Parsing Input
    1.1 Escape sequence identifies bold text
    1.2 Escape sequence identifies italic text
    1.3 Escape sequence identifies header text of multiple sizes
    1.4 Escape sequence identifies footnote
  2. Break text into lines
    2.1 Optimal line breaks are selected to minimize total trailing whitespace
    2.2 Line breaks can insert text when a break does not occur (for example insert a space)
    2.3 Line breaks can insert text at the end of a line it breaks (for example a hyphen)
    2.4 Text of varied fonts and sizes can be mixed in the same paragraph
  3. Break lines into pages
    3.1 Lines are fitted onto pages to minimise empty spece at the end of each page
    3.2 Footnotes are inserted at the bottom of the page
    3.3 A footnote must appear on the same page it's referenced
    3.4 Images can be inserted into the content
    3.5 Page numbers can optionally appear at the bottom of each page
    3.6 A contents page can be added which automatically locates relevant page numbers
    3.7 User-specified header text will appear at the top of each page
    3.8 Margin sizes can be controlled by the user
    3.9 Pages are written to a PDF file
    3.10 Fonts used are embedded into the PDF file
    3.11 Images used are embedded into the PDF file
END_OBJECTIVES

subheader "Prototype"
$markup_text << END_TEXT
In order to demonstrate the feasibility of the project, I wrote a python script
to generate a simple PDF file.
After successfully writing a one-page PDF file containing text of a built-in
font, I decided that the project was likely achievable.
END_TEXT

header "Documented Design"

subheader "Operating System"
$markup_text << END_TEXT
This project is intended to work on Unix-like machines only because it requires
use of Unix pipes.
END_TEXT

subheader "File Structure and Source Control"
$markup_text << END_TEXT
_git_ was used for the projects source control combined with _GitHub_ to backup
the repository in the cloud.
A secondary branch was created for the excessive code commenting and report
required by the NEA specification.

Source files are stored in the root 'typewriter' directory.
I did not feel the need to place them in a nested folder structure as they
are relate more procedurally than hierarchically.
In addition, it is easier and faster to access them when they are all in the
root folder.
Object files and binaries are also built into the same root directory.

The 'report' directory contains, source files and image for generating this
report along with symbolic links that point to the sample 'typeface' file and
'fonts' directory located in 'typewriter'.
These symbolic links are read by 'report.sh' when generating this report.
The 'test' directory contains test documents and similar symbolic links.
END_TEXT
tree .. --charset ASCI -I __* -I *.o | sed 's/^..$/typewriter/' | $markup_code

subheader "Executable Files"
$markup_text << END_TEXT
In order to modularise the typesetting process and make the project more
compatible with Unix systems, multiple independent executable files are built
each with responsibility of part of the document preparation process.
These executables communicate with each other through Unix pipes using these
well defined file formats: _'text specification', 'content', 'pages',
'contents'._

*tw.* tw (short for typewriter) is the program at the core of the project and
is often the last step of the typesetting process.
It converts the stdin stream of the _pages_ format to a PDF file.
The '-o' option can be used to specify where to output the PDF file.
The 'pages' format defines what graphic elements are to appear on each page and
where.
tw must embed fonts and images into the PDF file, as well as writing the text
content.
The value of this program is in the abstraction it provides over the complex
PDF file format.
It is much easier to write a program to generate the _pages_ format and then to
pipe that to tw than to write a program which generates PDF files directly.

*pager.* This executable reads the _content_ format from stdin, splits this
content into _pages_ which are written to stdout.
_contents_ is also written to a file. This contains a table of 'marks' (a
component of the _content_ format) and the pages which these marks are located.
The _contents_ file is essential in implementing a contents page.
Page breaks may only occur at optional breakpoints specified by the _content_
format.
Some specified points in _content_ must be located on a new page.
pager must consider footnotes and allocate sufficient space at the bottom of
the page for them to fit.
When both regular content and footnote content is added in between
optional breaks, both types of content must be located on the same page.
Command line options are provided which control page margins, page numbers
and where to output the contents file.

*contents.* This program converts the _contents_ table read from stdin into
_content_ which is written to stdout.
Each entry in the table is converted into a line of text starting with the
mark name, ending with the page number and padded with dots to achieve a
set character width.
A monospaced font is used so that the constant character width translates into
a constant line width.
Command line options can be used to set the character width and the font size.

*line_break.* This program reads _text specification_ from stdin, calculates
the optimal line breaks and writes _content_ to stdout.
The _text specification_ defines what text is to appear, with what font and
size, where line breaks can occur and where line breaks must occur.
Optional breaks can insert text depending on weather the break occurs.
For example, an optional break between two words inserts a space when no break
occurs and an optional break within a word inserts a hyphen when a break
occurs.
Line breaks are chosen to minimize the sum of surplus white space on each line.
The line break algorithm considers the entire paragraph when doing this; the
last word in a paragraph can affect the first line break.
Command line options can be used to set text align mode and line width.
Left, right, centre and justified align modes are supported.

*markup_text* and *markup_raw.* The markup programs read a human readable
markup language from stdin and write _content_ to stdout.
markup_raw writes the text 'as is', maintaining line breaks.
Font, maximum orphan lines and maximum widow lines can be set with a command line
options.
Orphans are lines of text that appear alone on the bottom of a page,
disconnected from the rest of their body of text.
Widows are isolated lines of text, alone at the top of a page.
markup_text invokes line_break to calculate the optimal line breaks for the
text it is given.
Text enclosed in stars (*) and underscores (_) is interpreted as bold and
italic text respectively.
Lines beginning with hashtags (#) are headers which change in font size
depending on the number of hashtags at the start of the line.
Lines beginning with a carrot symbol (^) are footnotes, the remainder of text
in the line is the content of the footnote associated with the last word before
the footnote.
Command line options are provided to control text width, size, align mode, line
spacing and paragraph spacing for both footnotes text and normal text.
END_TEXT

subheader "File Formats" ######################################################
subsubheader "Records"
$markup_text << END_TEXT
CSV files seem to be the standard way of encoding a table of strings in a text
file.
However, when fields include text that may contain escaped commas, it can be
confusing to read and appear cluttered.
For my project, I have developed an alternative format which encodes a variable
number of string fields on each line.
This _'records'_ format is used frequently in the projects text streams.

Lines are separated by a single new line character (LF not CRLF).
Empty lines and lines consisting of only spaces are ignored.
Spaces at the beginning and end of lines are ignored.
Fields are separated by one or more spaces.
A field consists of EITHER a sequence of characters none of which are spaces or
new lines OR a sequence of non new line characters enclosed in double quotation
marks (").
In each type of field, a backslash can be used to escape the next character.
For example a backslash followed by a space (\\\\\\\\ ) represents a space
literal that does not begin the next field and a backslash followed by a double
quotation mark (\\\\\\\\") represents a double quotation mark that does not end
the field.
A double backslash sequence (\\\\\\\\\\\\\\\\) encodes a backslash literal.
Fields beginning with a double quotation mark must be closed with another
double quotation mark before the end of the line.
END_TEXT

$markup_code << END_CODE
this line has 5 fields
this line has 4\ fields
"this line has 1 field"
field_0 "field 1" "field \"2\"." field\ 3
END_CODE

subsubheader "Pages"

$markup_text << END_TEXT
There are three modes in the _pages_ format: document, graphic, text.
Each mode interprets records differently.
The first line of a _pages_ file is interpreted in document mode.

*Document Mode Commands:*

*START PAGE* is the only valid document mode command.
It adds a new page to the document and enters graphic mode.
The graphic read in this graphic mode will define the page content with origin
(0, 0).
Parsing will return to document mode when graphic mode is exited.
When back in document mode, subsequent pages can be added with more 'START
PAGE' records.

*Graphic Mode Commands:*

*MOVE [x_offset] [y_offset]* This graphic command sets the _current position_
to the graphic origin plus (x_offset, y_offset).

*IMAGE [width] [height] [jpeg_file_name]* Draws a baseline DCT-based JPEG image
on the page at _current position_ with width and height measured in points.

*START GRAPHIC* Enters a new graphic mode with origin _current position._
This graphic is drawn on the same page.

*START TEXT* Enters text mode. The text read in text mode is painted on the
page at _current position._

*END* Exits graphic mode.

*Text Mode Commands:*

*FONT [font_name] [font_size]* Sets the active font.

*STRING [string]* Must appear after the first FONT command in this text mode.
Adds _string_ with the active font to the text to be drawn.

*SPACE [word_spacing]* Sets the _word spacing_ for subsequent STRING commands.
The word spacing is the additional width to add to space characters measured in
thousandths of points.
This is used in text justification.

*END* Exits text mode.

The following is an example _pages_ stream:
END_TEXT

$markup_code << END_CODE
START PAGE
MOVE 100 100
IMAGE 400 400 peppers.jpg
MOVE 60 766
START GRAPHIC
MOVE 100 0
START TEXT
FONT Regular 36
STRING "Title"
END
END
MOVE 60 742
START TEXT
FONT Regular 12
STRING "hello"
STRING " "
STRING "world"
STRING " "
SPACE 10000
STRING "this"
STRING " "
STRING "is"
STRING " "
STRING "some"
STRING " "
STRING "text"
END
END
START PAGE
MOVE 100 300
START TEXT
FONT Regular 12
STRING "document end"
END
END
END_CODE

subsubheader "Content"
$markup_text << END_TEXT
Content is parsed by a _pager_ program which splits the content into pages.
A _content_ stream consists of a sequence of content commands.
Some content commands are followed by a _pages_ graphic which must begin with
a record with first field 'START'.
This graphic continues until the number of records encountered with first field
'START' is equal to the number of records encountered with first field 'END'.
The following is a list of content commands and their function.

*flow [flow]* Sets the current _flow._ This may either be 'normal' or
'footnote'.

*box [height]* The following graphic with _height_ measured in points is to be
added to the current page.
The current _flow_ determines weather the graphic is to be placed in the main
body of content or in the footer.
If the box does not fit in the current page then a new page is added.

*glue [height]* If the previous box appears on the same page then the next
box must be vertically padded by _height_ points.

*opt_break* A page break may occur at this location
Every valid page break location must be explicitly defined.

*new_page* If any optional breaks already appear on the current page then the
following content must appear on a new page.

Example _content_ stream:
END_TEXT

$markup_code << END_CODE
box 12
START TEXT
FONT Regular 12
STRING "Hello"
STRING " "
STRING "world,"
STRING " "
STRING "this"
STRING " "
STRING "is"
END
opt_break
box 12
START TEXT
FONT Regular 12
STRING "an"
STRING " "
STRING "example."
END
opt_break
END_CODE

subsubheader "Contents"
$markup_text << END_TEXT
The _contents_ format defines information to be displayed in a contents page.
It is generated by a _pager_ and is the input to a _contents_ program to
generate contents page content.
The format consists of a number of records.
Each record must have 2 fields: the first is the name of some content to be
marked in the contents page, the second is the page number in which this
content appears.
The page number does not need be an integer, for example if roman numerals are
used for page numbers.
The marks will appear in the contents page in the same order they are defined
in the contents stream. The following is an example contents stream.
END_TEXT

$markup_code << END_CODE
"Preface" iv
"Introduction" vi
"Section 1" 1
"  Section 1.1" 2
"  Section 1.2" 5
"Section 2" "7"
END_CODE

subsubheader "Text Specification"
$markup_text << END_TEXT
The text specification format defines text before it is broken into lines.
The line_break program parses text specification and selects optimal line
breaks.
The format consists of a number of records, parsed sequentially.
The first field in each record is a 'command name' that defines the meaning of
subsequent fields in the record.
The following text lists each supported command and its purpose.

*FONT [font_name] [font_size]* This record must consists of 3 fields.
The first is the string 'FONT', to identify the command type.
The second is a string that identifies the font name as defined in the
_typeface_ file.
The third field must be an integer, the font size.
This command sets the font to be used in subsequent STRING and OPTBREAK
commands.

*STRING [string]* This command must appear after the first FONT command.
The string with the most recently set font is appended to the list of gizmos
which the line break algorithm will operate on.

*OPTBREAK [no_break_string] [at_break_string] [line_spacing]* Appends an
optional break gizmo.
If no break occurs here, the no_break_string with most recently set font is
inserted into the current line in-place.
If a break does occur here, the at_break_string is added onto the end of the
complete line, also with the most recently set font.
The new line will be vertically padded by line_spacing points from the previous
line.

*BREAK [line_spacing]* A line break must occur here.
The new line will be vertically padded by line_spacing points from the previous
line.

*MARK [identifier]* For the line that contains this mark: an additional record
is inserted into the content output inbetween the line's box graphic and the
content optional break.
This record consists of a carrot symbol (^) followed by the identifier.
The mark command is used for locating the line with which a footnote that is
associated with a word in that line must appear.

The following is an example of the _text specification_ format.
END_TEXT

$markup_code << END_CODE
FONT Regular 12
STRING "Hello"
OPTBREAK " " "" 0
STRING "world."
FONT Bold 12
OPTBREAK " " "" 0
STRING "bold"
OPTBREAK " " "" 0
STRING "text."
FONT Regular 12
OPTBREAK " " "" 0
STRING "hyphenat"
OPTBREAK "" "-" 0
STRING "ion."
BREAK 12
STRING "New"
OPTBREAK " " "" 0
STRING "paragraph*"
MARK cite_paragraph
OPTBREAK " " "" 0
STRING "with"
OPTBREAK " " "" 0
STRING "a"
OPTBREAK " " "" 0
STRING "footnote."
END_CODE

subsubheader "Typeface"
$markup_text << END_TEXT
The _typeface_ format is found in the typeface file (which must be named
'typeface').
This file must be located in the current working directory when a program that
needs it is run.
Its purpose is to map a font name to a font file.
Each record consists of two fields: the first is a font name, the second is a
file path to the corresponding font file.
The file path may be relative to the current working directory.

Example:
END_TEXT

$markup_code << END_CODE
Regular /usr/share/fonts/cmu.serif-roman.ttf
Italic cmu.serif-italic.ttf
Bold cmu.serif-bold.ttf
Monospace cmu.typewriter-text-regular.ttf
END_CODE

subheader "Data Structures" ###################################################
subsubheader "Dynamic Buffer"
$markup_code << END_CODE
struct dbuffer {
  int size, allocated, increment;
  char *data;
};
void dbuffer_init(struct dbuffer *buf, int initial, int increment);
void dbuffer_putc(struct dbuffer *buf, char c);
void dbuffer_printf(struct dbuffer *buf, const char *format, ...);
void dbuffer_free(struct dbuffer *buf);
END_CODE
$markup_text << END_TEXT
This structure is usefull in string building.
_data_ points to a region of memory allocated on the heap _allocated_ bytes in
size.
_size_ measures how many bytes of _data_ are being used.
As data is added to the buffer, if more than _allocated_ bytes are needed then
more bytes are allocated in increments of _increment._
_dbuffer_printf_ and _dbuffer_putc_ functions append new characters to the
dynamic buffer.
END_TEXT

subsubheader "Record"
$markup_code << END_CODE
struct record {
  struct dbuffer string;
  int field_count;
  int fields_allocated;
  const char **fields;
};
void init_record(struct record *record);
void begin_field(struct record *record);
int parse_record(FILE *file, struct record *record);
int find_field(const struct record *record, const char *field_str);
void free_record(struct record *record);
END_CODE
$markup_text << END_TEXT
The record structure stores a variable number of strings which are referred to
as 'fields'.
_strings_ is a dynamic buffer that contains each field separated and ended with
null characters.
_fields_ is a pointer to a heap-allocated array of pointers to the first byte
of each field in _strings._
_fields_ has been allocated _allocated_ bytes on the heap, this amount may
increase as more fields are added.
There are _field_count_ valid and unique entries in _fields._
END_TEXT

subsubheader "Font Info"
$markup_code << END_CODE
struct font_info {
  int units_per_em;
  int x_min;
  int y_min;
  int x_max;
  int y_max;
  int long_hor_metrics_count;
  int cmap[256];
  int char_widths[256];
};
int read_ttf(FILE *file, struct font_info *info);
END_CODE
$markup_text << END_TEXT
_font_info_ contains the data parsed from a true type font file.
Character widths are measured by thousandths of points the character would
occupy if drawn with font size 1.
END_TEXT

subsubheader "Line Break Gizmos"
xref_line_break_gizmos="\$section_counter.\$subsection_counter.\$subsubsection_counter"
$markup_code << END_CODE
struct gizmo {
  int type;
  struct gizmo *next;
  char _[];
};

struct text_gizmo {
  int type; /* GIZMO_TEXT */
  struct gizmo *next;
  int width;
  struct style style;
  char string[];
};

struct break_gizmo {
  int type; /* GIZMO_BREAK */
  struct gizmo *next;
  int force_break, total_penalty, spacing, selected;
  struct break_gizmo *best_source;
  struct style style;
  int no_break_width, at_break_width;
  char *no_break, *at_break;
  char strings[];
};

struct mark_gizmo {
  int type; /* GIZMO_MARK */
  struct gizmo *next;
  char string[];
};
END_CODE

$markup_text << END_TEXT
The gizmo linked list is the data structure used in the line breaking
algorithm.
A _gizmo_ is a variable-size region of heap-allocated memory.
The first _sizeof(int)_ bytes are used to identify the type of gizmo: text
gizmo, break gizmo or mark gizmo.
The next _sizeof(char *)_ bytes are a pointer to the next gizmo in the list.
The meaning of subsequent bytes is dependent on the type of the gizmo.
Each gizmo type ends with a variable number of characters which may be used as
a place to store null-terminated strings which can be pointed to in the gizmo.
In the case of the _text_gizmo,_ and _mark_gizmo_ the first null-terminated
string is the text of concern.
The break gizmo represents a node in the line breaking algorithm, it therefore
contains a number of variables relevant in the algorithm.
All gizmo widths are measured in thousandths of points.
END_TEXT

subheader "Key Algorithms" ####################################################
subsubheader "Line Breaker"
$markup_text << END_TEXT
The line breaking algorithm used in this project is partially based of that
presented in Donald E Knuth's 'Breaking Paragraphs into Lines (1981)'
[$cite_breaking_paragraphs_into_lines].

Input text is modelled by a gizmo list defined in section
\$xref_line_break_gizmos.
Consider an directed acyclic graph (DAG) where each vertex is a potential line
break and each edge a feasible line of text.
The source vertex is the hypothetical line break preceding the body of text and
end sink vertex represents the line break after the last item of text.
An edge is feasible only if the sum of its text width is less than the
maximum line width.
Edges are weighted by the difference between the line's text width and the
maximum line width.
The problem of finding optimal line breaks (that minimises trailing white
space) is equivalent to finding the shortest path through this graph from the
source to the sink.
Therefore, to find optimal line breaks, the algorithm must identify feasible
lines, evaluate the weight of these lines and find the shortest path through
the graph.
These three tasks can be achieved in a single pass.
END_TEXT

echo "box 0"
echo "START GRAPHIC"
echo "MOVE 206 -235"
echo "IMAGE 184 235 dag.jpg"
echo "END"
(markup_text.py -w 200 -s 10 -a j -A l | sed 's/^opt_break$//') << END_SIDE
The shortest path of a DAG can be computed by relaxing each vertex in order of
a topological sort [$cite_introduction_to_algorithms].
As each edge can only connect an earlier line break to a later one, and because
line breaks in the gizmo list appear in text order, the break gizmos are
already necessarily in topological order.
This means that line breaks can be relaxed in the order they appear in the
gizmo list.
Relaxing a break involves finding each feasible line that may follow this
break, computing the weight of the line and if the destination break does not
claim a shorter path, then update the destination breaks shortest path claim to
that which ends with this edge.

The diagram *(right)* illustrates the solved graph for the text
"Hello world! I am text." with a mono-spaced 1 point font fitting in lines of
max width 12.
Edges are labeled with their weight and the text contained in the line they
represent.
As you can see, the shortest route from top to bottom is through,
"Hello world!" and "I am text." with a total weight of 2.
This means that the optimal line break set is the single break after "world!".
END_SIDE
echo "glue 10"

subsubheader "Page Breaker"
$markup_text << END_TEXT
The purpose of the page breaker is to divide _content_ into _pages._
Boxes and glue (both are a type of 'gizmo') are collected sequentially from the
input _content_ into a normal bin and into a footnote bin.
The chosen bin depends on the argument of the most recent flow command.
When an optional page break or new page command is reached, if the current page
has sufficient space to fit the pending gizmos in both bins, then they are
added to the current page.
Otherwise, the pending gizmos are added to a new page which becomes the
current.
After reading a new page command and fitting the pending gizmos, a new page is
added.
When a mark command is encountered, the page number of the current page is
added to the contents file.

To determine weather a set of gizmos fits on a page, the height of the content
of each bin is calculated by summing the height of all gizmos in that bin
excluding trailing glue gizmos.
If the total height is smaller than the height of the page's max content height
(as calculated from the page height and vertical margins) then the gizmos fit.
END_TEXT

subsubheader "Record Parser"
($markup_text | sed 's/^opt_break$//') << END_TEXT
The python implantation of record parsing uses the following regular
expression:
END_TEXT
$markup_code << END_CODE
[^"\s]\S*|".*?[^\\]
END_CODE
$markup_text << END_TEXT
However, I take more pride in my C code and so in the name of efficiency will
be writing a custom parser.
Characters are to be parsed sequentially using the following state machine of
10 states.
END_TEXT
echo "box 198"
echo "START GRAPHIC"
echo "IMAGE 390 198 state_machine.jpg"
echo "END"
$markup_text << END_TEXT
Parsing begins in the 'Start' state. 'EOF' indicated the End Of File.
When a field is started, a pointer to the current end of the _string_ buffer
is added to the _fields_ array.
When a character is added to a field, the character is appended to _string._
When a field is ended, a zero byte is appended to _string._
When a terminating or error state is reached, parsing stops immediately.
The result of parsing is fields separated and ended in zero bytes in the
_string_ dynamic buffer and pointers to the beginning of each field in the
_fields_ array.
END_TEXT

subheader "Example Document" ##################################################
$markup_text << END_TEXT
This section will consider the following shell command and explain how it
should work.
END_TEXT

$markup_code << END_CODE
echo "Hello world" | markup_text.py -w 60 | pager.py | tw
END_CODE

$markup_text << END_TEXT
First, "Hello world" is piped into markup_text.py with content width set to 60
points.
The string will be parsed and converted into a _text_specification._
markup_text.py will automatically pass this to a new instance of the line_break
program.
The following is the _text_specification._
END_TEXT

$markup_code << END_CODE
FONT Regular 12
STRING "Hello"
OPTBREAK " " "" 0
STRING "world"
END_CODE

$markup_text << END_TEXT
The line_break program will respond with the following _content_ which
markup_text.py will output.
END_TEXT

$markup_code << END_CODE
box 12
START TEXT
FONT Regular 12
STRING "Hello"
END
opt_break
box 12
START TEXT
FONT Regular 12
STRING "world"
END
opt_break
END_CODE

$markup_text << END_TEXT
pager.py will interpret this _content,_ and split it into _pages_ and output
the following.
END_TEXT

$markup_code << END_CODE
START PAGE
MOVE 102 705
START TEXT
FONT Regular 12
STRING "Hello"
END
MOVE 102 693
START TEXT
FONT Regular 12
STRING "world"
END
END
END_CODE

$markup_text << END_TEXT
Finally, tw reads the _pages_ and writes the pdf that is described.
In this case, a single page with "Hello" on the first line and "world" on the
second.
END_TEXT

header "Technical Solution" ###################################################

subheader "Techniques Used"

$markup_text << END_TEXT
*Complex File Formats*
Parsing for the hierarchical _pages_ format is implemented in 'tw.c'.
I designed this format specifically for this project; it is defined in
Documented Design.

*Recursive Algorithm*
'tw.c' parses the _pages_ format by recursivly calling _parse_graphic._

*Dynamic Buffer*
'tw.h' defines _struct dbuffer_ and 'dbuffer.c' implements functions for it.
It allocates a region of memory on the heap.
When more memory is required for the buffer, it is reallocated.
_dbuffer_printf_ can be used to print a formatted string directly to the end of
the buffer.

*String List*
'tw.h' defines _struct record_ and 'record.c' implements functions for it.
This record stores a list of strings (fields) on the heap and keeps pointers
to each of these strings in an array also stored on the heap.
_find_field_ can be used to search for a string in the record.

*Linked List*
'line_break.c' defines _struct typeface_ and the _gizmo_ structures.
These are linked lists.
Functions to manage these linked lists content and memory are also defined:
_open_typeface, free_typeface, parse_gizmos, free_gizmos,_ etc.

*State Machine*
'record.c' uses a state machine to parse records.
_enum ParseState_ defines the states; _parse_record_ implements the state
machine and state transitions.

*Polymorphism*
'line_break.c' defines _struct text_gizmo, struct break_gizmo_ and _struct
mark_gizmo._
Each of these structures share a number of starting bytes with the same meaning
(defined by _struct gizmo_ ).
This means that a function can be passed a generic _struct gizmo_ which can be
cast to the correct gizmo type based of _type._
Polymorphism is also used in 'pager.py', where _Box_ and _Glue_ implement the
same methods.

*Inheritance*
Inheritance is used in 'markup_text.py' to define a subtypes of TextStream
can be defined to change how a stream of text if parsed and converted into
content.

*Shortest Path in Directed Acyclic Graph*
This shortest path algorithm is similar to Dijkstra's, but is faster since it
takes advantage of the graphs acyclic nature and topological sort.
It finished in O(e+v) time where _e_ is number of edges and _v_ is number of
vertices.
It is implemented in 'line_break.c' to determine the optimal line breaks that
minimise the total trailing whitespace at the end of each line.

*Exception Handling*
Thorough exception handling is used throughout the codebase.
Examples can be found in most source files.
One specific example is at the end of _parse_record_ in 'record.c' where
unterminated strings and escapes are handled.

*File Paths Parameterised*
'tw.c' provides a command line argument to change the output PDF filename.
'pager.py' provides a command line argument to change the output 'contents'
filename.

*Symbolic Links*
Symbolic links are used in the 'test' and 'report' folders to resolve the
typeface file and fonts directory.
END_TEXT

subheader "Source Code"

function embed_source {
  basename "\$1" | awk '{printf "*%s*\n", \$0}' | markup_text.py -w $body_width -s 12 -a l | sed 's/^opt_break$//'
  echo "glue 3"
  $mark "    \$(basename \$1)"
  (sed 's/\t/    /g' | awk '{printf "%03d %s\n", ++i, \$0}' | markup_raw.py -o 8 -w 4 -s 8) < \$1
  echo "glue 10"
}

embed_source "../Makefile"
embed_source "../tw.h"
embed_source "../tw.c"
embed_source "../line_break.c"
embed_source "../dbuffer.c"
embed_source "../record.c"
embed_source "../pdf.c"
embed_source "../jpeg.c"
embed_source "../ttf.c"
embed_source "../utils.c"

embed_source "../utils.py"
embed_source "../contents.py"
embed_source "../markup_raw.py"
embed_source "../markup_text.py"
embed_source "../pager.py"

header "Testing" ##############################################################

$markup_text << END_TEXT
Testing in the early stages of my project was achieved with the 'doc.sh' shell
script.
This script used the project's executables to generate a PDF that demonstrated
all the features of current system.
After each new change change, I could verify the program worked by running the
shell script and inspecting the PDF.
The script was updated to use new features as they were added.

The following code listing shows the latest version of 'doc.sh'.
It includes a large amount of _lorem ipsum_ (Latin sample text that has been
traditionally used to test text-processing systems).
It also has a contents page, an image and a custom page inserted at the end of
the document.
Footnotes with a very large font size are used to demonstrate that they will
never overlap with the main page content.
END_TEXT

$markup_code < ../test/doc.sh

$markup_text << END_TEXT
After my project reached a sufficient set of features, I wrote a shell script
to typeset and generate a PDF of the most recent draft of the NEA report.
All subsequent report drafts (including this final one) were typeset with this
project's software.
Practical use of the software helped me uncover bugs and identify important new
features.

In order to test the output document's compliance with the 1.7 PDF standard, I
used a web based PDF validation tool
^* https://www.pdf-online.com/osa/validate.aspx
throughout the project's development.

The following shell script generates _content_ for the _pager_ program.
Below the shell script source, a page is included with content generated by the
script.
This page is designed to demonstrate and text the capability of the software.
END_TEXT

$markup_code < demo_content.sh
echo "new_page"
./demo_content.sh
echo "new_page"

header "Evaluation" ###########################################################

subheader "Requirements Met"

$markup_text << END_TEXT
The project has met it's initial requirements as defined by the project
statement and list of objectives.
My experience in typesetting this report with the software has been good; it is
clear to me that I have produced an effective and useful tool.
END_TEXT

subheader "Improvements"

$markup_text << END_TEXT
In its current state, the software considers line breaking and page breaking
to be two separate problems with a separate implantation.
This simplifies each individual problem but also limits the software capability
to solve more complex problems in which line breaking and page breaking can not
be considered separately (perhaps the width of text is constrained by where it
falls on a page).
I envision a so called 'universal content breaker' which is given a complete
model of all content to be typeset and a well defined objective function.
This universal content breaker will handle both line breaks, page breaks and
any other type of content separation that is required in a single optimization
problem (most likely modeled by a shortest path problem as the line breaking
currently is).
By centralizing the typesetting problem, the document content and typesetting
objective can be defined more rigorously without being limited by pre-selected
line breaks (as the pager currently is).
This would also reduce the number of file formats that the user must know,
potentially making the product easier to use.
END_TEXT

subheader "Feedback"

$markup_text << END_TEXT
I spoke to the third-party of the project - Anthony Ceponis - about the
finished project.
We looked at the shell script that builds the report and discussed how my new
typesetting system compares to alternatives such as LaTeX.
The following is an extract of Anthony's comments.

_I definitely think that this is a huge improvement over LaTeX. The main reason
I prefer this new system you have built is because much less syntax is needed
to achieve the exact same things without jeopardising the clarity of the
markup. On the note of the markup itself, I really like how human readable it
is. For example, there is a very limited use of non alpha numeric symbols (like
angle brackets which are abused in html) which I think is a huge plus.

I would not really compare this to something like Google docs because I think
they are both built for different things. Google docs is very much a 'user'
product rather than a developer oriented product which is why I don't think it
would be that appropriate to compare them. I can easily see your typesetting
system being used by people like web-developers to create complex documents
with complex structures or even for simpler use like storing
structured/formatted blogs on a database.

Obviously this project is still very young so my suggested improvements would
have probably been incorporated over time anyways but I would be interested to
see how things like mathematical symbols would be represented (e.g. integral
and sigma signs) in the markup because currently I use latex for this and I am
not a huge fan of how it works currently. I also dislike the bracketing system
in latex (the height of brackets should be able to adjust automatically without
the need for extra code). Some more complicated features like tables would also
be interesting to see. Clearly the only improvements I have are to just add
more features but as of now, I don't have any complaints about changing any
existing markup systems in your project._

I agree with Anthony's remark that the project is more suitable for developers
than for the average computer user.
Perhaps an optional, additional layer of abstraction could be provided to
simplify basic usage of the system for novice Unix users.

I disagree however, with Anthony's belief that the project simply needs to
'just add more features'.
I believe that the best way to improve on the project is to redesign the
fundamental system to give the user a set of simple cohesive tools which can be
combined to produce more complex features such as tables.

Overall, I think Anthony's comments reflect my assertion that the project
fulfills its original goals, though it could be improved further in some ways
if more time was available.
END_TEXT


END_CONTENT

(sh | tw -o nea.pdf) << END_DOCUMENT

(sh | pager.py) << END_COVER
echo "glue 180"
echo "NEA Report" | markup_text.py -w $body_width -a c -s 24
echo "glue 4"
echo "Christopher Lang 1132" | markup_text.py -w $body_width -s 12 -a c
echo "Typesetting System" | markup_text.py -w $body_width -s 12 -a c
echo "2022-2023" | markup_text.py -w $body_width -s 12 -a c
END_COVER

(sh | pager.py) << END_CONTENTS_PAGE
echo "Contents" | markup_text.py -w $body_width -s 26 -a l
echo "glue 20"
# cmu monospace font width is 524
contents.py -c 62 < .contents
END_CONTENTS_PAGE

cat .content_pages
END_DOCUMENT

