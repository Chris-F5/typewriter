#include "tw.h"

static void write_pdf_string(struct bytes *bytes, const char *string);
static void write_primitive(struct bytes *bytes,
    const struct pdf_primitive *primitive);
static void write_instruction(struct bytes *bytes,
    const struct pdf_content_instruction *instruction);

static void
write_pdf_string(struct bytes *bytes, const char *string)
{
  bytes_printf(bytes, "(");
  for (; *string; string++) {
    if (*string == '(' || *string == ')' || *string == '\\')
      bytes_printf(bytes, "\\");
    bytes_printf(bytes, "%c", *string);
  }
  bytes_printf(bytes, ")");
}

static void
write_primitive(struct bytes *bytes, const struct pdf_primitive *primitive)
{
  switch (primitive->type) {
  case PDF_NUMBER:
    bytes_printf(bytes, "%g", primitive->data.num);
    break;
  case PDF_STRING:
    write_pdf_string(bytes, primitive->data.str);
    break;
  case PDF_NAME:
    bytes_printf(bytes, "/%s", primitive->data.str);
    break;
  }
}

static void
write_instruction(struct bytes *bytes,
    const struct pdf_content_instruction *instruction)
{
  int i;
  for (i = 0; i < instruction->operand_count; i++) {
    write_primitive(bytes, &instruction->operands[i]);
    bytes_printf(bytes, " ");
  }
  bytes_printf(bytes, "%s\n", instruction->operation);
}

void
write_graphic(struct bytes *bytes, const struct pdf_graphic *graphic)
{
  struct pdf_content_instruction *instruction;
  for (
      instruction = graphic->first;
      instruction;
      instruction = instruction->next)
    write_instruction(bytes, instruction);
}
