import sys, re

columns = 80
rows = 57

def export_pdf(pages):
  import fpdf
  font_size = 10
  line_gap = 2
  font = 'Courier'
  a4_dims = (595, 842)

  pdf = fpdf.FPDF('P', 'pt', 'A4')
  pdf.set_font(font, size=font_size)
  body_dims = (pdf.get_string_width('a'*columns), (font_size+line_gap)*rows)
  hmargin = (a4_dims[0] - body_dims[0])/2
  vmargin = (a4_dims[1] - body_dims[1])/2
  pdf.set_left_margin(hmargin)
  pdf.set_right_margin(hmargin)
  pdf.set_top_margin(vmargin)
  for page in pages:
    pdf.add_page()
    for line in page:
        pdf.cell(body_dims[0], font_size + line_gap, line)
        pdf.ln()
  pdf.output('output.pdf', 'F')


pages = []
page = []
code = ''
title = ''
date = ''
author = ''
def add_page():
  global pages, page
  left_padding = (columns - len(title))//2
  right_padding = columns - len(title) - left_padding
  header = code + ' '*(left_padding-len(code)) + title + ' '*(right_padding-len(author)) + author
  page_number = f"[Page {len(pages) + 1}]"
  footer_whitespace = columns - len(author) - len(page_number)
  footer = author + " "*footer_whitespace + page_number
  page.insert(0, header)
  page.insert(1, "")
  for i in range(rows-4-len(page)):
    page.append("")
  page.append("")
  page.append(footer)
  pages.append(page)
  page = []

for line in sys.stdin:
  line = line.rstrip()
  if re.search(r'!(-|=)+', line):
    add_page()
    continue
  if r := re.search(r'!code (.+)', line):
    code = r.group(1)
    continue
  if r := re.search(r'!title (.+)', line):
    title = r.group(1)
    continue
  if r := re.search(r'!date (.+)', line):
    date = r.group(1)
    continue
  if r := re.search(r'!author (.+)', line):
    author = r.group(1)
    continue
  if len(page) >= rows-4:
    add_page()
  page.append(line)
if len(page) > 0:
  add_page()

export_pdf(pages)
