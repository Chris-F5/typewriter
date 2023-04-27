/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "tw.h"

/* Text align modes. */
enum align {
  ALIGN_LEFT,
  ALIGN_RIGHT,
  ALIGN_CENTRE,
  ALIGN_JUSTIFIED,
};

/* Types for (struct gizmo)->type. */
enum gizmo_type {
  GIZMO_TEXT,
  GIZMO_BREAK,
  GIZMO_MARK,
};

/* Linked list, each struct maps one _font_name_ to one _font_info_. */
struct typeface {
  /* Font names cant be longer than 255 characters. */
  char font_name[256];
  struct font_info font_info;
  /* Next font in the linked list. */
  struct typeface *next;
};

/* A text style. */
struct style {
  int font_size;
  char font_name[256];
};

/* Polymorphic linked list. _type_ defines the meaning of _char _[]_ bytes. */
struct gizmo {
  int type;
  struct gizmo *next;
  /* Any number of bytes can follow (gizmo specific data). */
  char _[];
};

struct text_gizmo {
  int type; /* GIZMO_TEXT */
  struct gizmo *next;
  int width;
  struct style style;
  /* A string of any number of bytes can follow. */
  char string[];
};

struct break_gizmo {
  int type; /* GIZMO_BREAK */
  struct gizmo *next;
  int force_break, total_penalty, spacing, selected;
  struct break_gizmo *best_source;
  struct style style;
  int no_break_width, at_break_width;
  /* Pointers to inside _strings_. */
  char *no_break, *at_break;
  /* Any number of bytes can follow (for storing strings). */
  char strings[];
};

struct mark_gizmo {
  int type; /* GIZMO_MARK */
  struct gizmo *next;
  char string[];
};

static struct typeface *open_typeface(FILE *typeface_file);
static void free_typeface(struct typeface *typeface);
static int get_text_width(const char *string, const struct style *style,
    const struct typeface *typeface);
static struct gizmo *parse_gizmos(FILE *file, const struct typeface *typeface);
static void free_gizmos(struct gizmo *gizmo);
static void consider_breaks(struct gizmo *gizmo, int source_penalty,
    struct break_gizmo *source, int line_width);
static void optimise_breaks(struct gizmo *gizmo, int line_width);
static void print_text(struct dbuffer *buffer, struct style *style,
    const struct style *new_style, const char *string, int *spaces);
static void print_gizmos(FILE *output, struct gizmo *gizmo, int line_width,
    int align);

/* 
 * Parse the _typeface_file_ and return a linked list of fonts in the typeface.
 */
static struct typeface *
open_typeface(FILE *typeface_file)
{
  struct typeface *first_font;
  struct typeface **next_font;
  int parse_result;
  struct record record;
  FILE *font_file;
  first_font = NULL;
  next_font = &first_font;
  init_record(&record);
  /* Loop through all records in the typeface file. */
  for (;;) {
    parse_result = parse_record(typeface_file, &record);
    /* If end of file reached, stop. */
    if (parse_result == EOF)
      break;
    /* If failed to parse this record, skip it. */
    if (parse_result)
      continue;
    if (record.field_count != 2) {
      fprintf(stderr, "Typeface records must have exactly 2 fields.");
      continue;
    }
    /*
     * Fonts can't be longer than 255 characters because that would overflow
     * their buffer.
     */
    if (strlen(record.fields[0]) >= 256) {
      fprintf(stderr,
          "Typeface file contains font name that is too long '%s'.\n",
          record.fields[0]);
      /* Go to next record. */
      continue;
    }
    /* Check the font name does not contain any illegal characters. */
    if (!is_font_name_valid(record.fields[0])) {
      fprintf(stderr, "Typeface file contains invalid font name '%s'.\n",
          record.fields[0]);
      continue;
    }
    /* Open the font file that the typeface record references. */
    font_file = fopen(record.fields[1], "r");
    if (font_file == NULL) {
      fprintf(stderr, "Failed to open ttf file '%s': %s\n",
          record.fields[1], strerror(errno));
      continue;
    }
    /* Allocate space on the heap for the next node in the linked list. */
    *next_font = xmalloc(sizeof(struct typeface));
    /* Copy the font name into the new node. */
    strcpy((*next_font)->font_name, record.fields[0]);
    /* Try to parse the font file to get the font info. */
    if (read_ttf(font_file, &(*next_font)->font_info)) {
      fprintf(stderr, "Failed to parse ttf file: '%s'\n", record.fields[1]);
      free(*next_font);
    } else {
      /* Add this font to the linked list. */
      next_font = &(*next_font)->next;
    }
    /* Close the font file. */
    fclose(font_file);
  }
  /* End the linked list here. */
  *next_font = NULL;
  free_record(&record);
  /* Return the first node in the linked list. */
  return first_font;
}

/* Free a typeface linked list. */
static void
free_typeface(struct typeface *typeface)
{
  struct typeface *next;
  /* Loop through every node in the linked list and free it. */
  while (typeface) {
    next = typeface->next;
    free(typeface);
    typeface = next;
  }
}

/*
 * Compute the width of _string_ in _style_ with _typeface_ measured in
 * thousandths of points.
 */
static int
get_text_width(const char *string, const struct style *style,
    const struct typeface *typeface)
{
  int width;
  const struct typeface *font;
  font = typeface;
  /* Loop over fonts until a name matches. */
  while (font && strcmp(font->font_name, style->font_name))
    font = font->next;
  /* If no names matched. */
  if (font == NULL) {
    fprintf(stderr, "Typeface does not include font: '%s'\n", style->font_name);
    font = typeface;
  }
  width = 0;
  /* Loop over every character in string and add it's width. */
  for (; *string; string++)
    width += font->font_info.char_widths[(unsigned char)*string];
  return width * style->font_size;
}

/*
 * Parse text gizmos from the text specification _file_ and return them as a
 * linked list.
 */
static struct gizmo *
parse_gizmos(FILE *file, const struct typeface *typeface)
{
  struct gizmo *first_gizmo;
  struct gizmo **next_gizmo;
  struct style current_style;
  int parse_result, arg1;
  struct record record;
  /* An empty linked list is just a NULL pointer. */
  first_gizmo = NULL;
  /* _next_gizmo_ points to the end of the linked list. */
  next_gizmo = &first_gizmo;
  /* Initialise _current_style_. */
  current_style.font_name[0] = '\0';
  current_style.font_size = 0;
  init_record(&record);
  /* For each record in _file_. */
  for (;;) {
    parse_result = parse_record(file, &record);
    /* If reach end of file then stop. */
    if (parse_result == EOF)
      break;
    /* If failed to parse record then skip it. */
    if (parse_result)
      continue;
    /* The first field in the record is the command type. */
    if (strcmp(record.fields[0], "STRING") == 0) {
      /* STRING [STRING] */
      if (record.field_count != 2) {
        fprintf(stderr, "Text STRING command must have 1 option.\n");
        continue;
      }
      /* If the font name is still empty from initialisation. */
      if (current_style.font_name[0] == '\0') {
        fprintf(stderr,
            "Text STRING command can't be called without FONT set.\n");
        /* Next record. */
        continue;
      }
      /*
       * Allocate space for the next gizmo in the linked list. It must contain
       * the text_gizmo struct followed by the string specified in the record.
       */
      *next_gizmo = xmalloc(sizeof(struct text_gizmo)
          + strlen(record.fields[1]) + 1);
      /* Initialise the new text gizmo.  */
      (*next_gizmo)->type = GIZMO_TEXT;
      (*next_gizmo)->next = NULL;
      /* Find the string width. */
      ((struct text_gizmo *)*next_gizmo)->width
        = get_text_width(record.fields[1], &current_style, typeface);
      ((struct text_gizmo *)*next_gizmo)->style = current_style;
      /* Copy the string into the gizmo. */
      strcpy(((struct text_gizmo *)*next_gizmo)->string, record.fields[1]);
      /* Update _next_gizmo_ to point to the end of the linked list. */
      next_gizmo = &(*next_gizmo)->next;
      /* Next record. */
      continue;
    }
    if (strcmp(record.fields[0], "FONT") == 0) {
      /* FONT [FONT_NAME] [FONT_SIZE] */
      if (record.field_count != 3) {
        fprintf(stderr, "Text FONT command must have 2 options.\n");
        continue;
      }
      /* Validate font name. */
      if (strlen(record.fields[1]) >= 256) {
        fprintf(stderr, "Text font name too long: '%s'\n", record.fields[1]);
        continue;
      }
      if (!is_font_name_valid(record.fields[1])) {
        fprintf(stderr, "Text font name contains illegal characters: '%s'\n",
            record.fields[1]);
        continue;
      }
      /* Update the current style. */
      strcpy(current_style.font_name, record.fields[1]);
      if (str_to_int(record.fields[2], &current_style.font_size)) {
        fprintf(stderr, "Text FONT command's 2nd option must be integer.\n");
        current_style.font_size = 12;
      }
      /* Next record. */
      continue;
    }
    if (strcmp(record.fields[0], "OPTBREAK") == 0) {
      /* OPTBREAK [NO_BREAK_STRING] [AT_BREAK_STRING] [SPACING] */
      if (record.field_count != 4) {
        fprintf(stderr, "Text OPTBREAK command must have 3 options.\n");
        continue;
      }
      /* Convert the [SPACING] option to an integer. */
      if (str_to_int(record.fields[3], &arg1)) {
        fprintf(stderr,
            "Text OPTBREAK command's 3rd option must be integer.\n");
        continue;
      }
      /*
       * Allocate memory for the new gizmo. It must contain the break_gizmo
       * struct, the [NO_BREAK_STRING] and the [AT_BREAK_STRING].
       */
      *next_gizmo = xmalloc(sizeof(struct break_gizmo)
          + strlen(record.fields[1])
          + strlen(record.fields[2]) + 2);
      /* Initialise the new break gizmo. */
      (*next_gizmo)->type = GIZMO_BREAK;
      (*next_gizmo)->next = NULL;
      ((struct break_gizmo *)*next_gizmo)->spacing = arg1;
      /* And OPTBREAK does not force a break here. */
      ((struct break_gizmo *)*next_gizmo)->force_break = 0;
      /* We do not yet know if this gizmo will be selected. */
      ((struct break_gizmo *)*next_gizmo)->selected = 0;
      /*
       * Set the total penalty to INT_MAX so the first route to this break
       * overwrites it.
       */
      ((struct break_gizmo *)*next_gizmo)->total_penalty = INT_MAX;
      /* _best_source_ is used for the shortest path algorithm. */
      ((struct break_gizmo *)*next_gizmo)->best_source = NULL;
      ((struct break_gizmo *)*next_gizmo)->style = current_style;
      /* Get the text width for the [NO_BREAK_STRING] and [AT_BREAK_STRING]. */
      ((struct break_gizmo *)*next_gizmo)->no_break_width
        = get_text_width(record.fields[1], &current_style, typeface);
      ((struct break_gizmo *)*next_gizmo)->at_break_width
        = get_text_width(record.fields[2], &current_style, typeface);
      /*
       * Set the _no_break_ and _at_break_ pointers to point into the correct
       * location in _strings_.
       */
      ((struct break_gizmo *)*next_gizmo)->no_break
        = ((struct break_gizmo *)*next_gizmo)->strings;
      ((struct break_gizmo *)*next_gizmo)->at_break
        = ((struct break_gizmo *)*next_gizmo)->strings
        + strlen(record.fields[1]) + 1;
      /* Copy the strings into the gizmo. */
      strcpy(((struct break_gizmo *)*next_gizmo)->no_break,
          record.fields[1]);
      strcpy(((struct break_gizmo *)*next_gizmo)->at_break,
          record.fields[2]);
      /* Update _next_gizmo_ to point to the new end of the linked list. */
      next_gizmo = &(*next_gizmo)->next;
      /* Next record. */
      continue;
    }
    if (strcmp(record.fields[0], "BREAK") == 0) {
      /* BREAK [SPACING] */
      if (record.field_count != 2) {
        fprintf(stderr, "Text BREAK command must take 1 option.\n");
        continue;
      }
      if (str_to_int(record.fields[1], &arg1)) {
        fprintf(stderr, "Text BREAK command's 1st option must be integer.\n");
        continue;
      }
      /* Allocate memory for this gizmo. Plus 1 for the empty string. */
      *next_gizmo = xmalloc(sizeof(struct break_gizmo) + 1);
      /* Initialise this gizmo. */
      (*next_gizmo)->type = GIZMO_BREAK;
      (*next_gizmo)->next = NULL;
      ((struct break_gizmo *)*next_gizmo)->spacing = arg1;
      /* BREAK command forces a break here. */
      ((struct break_gizmo *)*next_gizmo)->force_break = 1;
      /* _selected_ is set to zero. Adjacent BREAKS are not both selected. */
      ((struct break_gizmo *)*next_gizmo)->selected = 0;
      ((struct break_gizmo *)*next_gizmo)->total_penalty = INT_MAX;
      ((struct break_gizmo *)*next_gizmo)->best_source = NULL;
      ((struct break_gizmo *)*next_gizmo)->style = current_style;
      /* There is no 'at break' or 'no break' text. */
      ((struct break_gizmo *)*next_gizmo)->no_break_width = 0;
      ((struct break_gizmo *)*next_gizmo)->at_break_width = 0;
      /* Both _no_break_ and _at_break_ point to an empty string. */
      ((struct break_gizmo *)*next_gizmo)->no_break
        = ((struct break_gizmo *)*next_gizmo)->at_break
        = ((struct break_gizmo *)*next_gizmo)->strings;
      ((struct break_gizmo *)*next_gizmo)->strings[0] = '\0';
      /* Update _next_gizmo to point to the new end of the linked list. */
      next_gizmo = &(*next_gizmo)->next;
      /* Next record. */
      continue;
    }
    if (strcmp(record.fields[0], "MARK") == 0) {
      /* MARK [MARK_STRING] */
      if (record.field_count != 2) {
        fprintf(stderr, "Text MARK command must have 1 option.\n");
        continue;
      }
      /* Allocate space for this gizmo and for its mark string. */
      *next_gizmo = xmalloc(sizeof(struct mark_gizmo)
          + strlen(record.fields[1]) + 1);
      /* Initialise this gizmo. */
      (*next_gizmo)->type = GIZMO_MARK;
      (*next_gizmo)->next = NULL;
      /* Copy the mark string into the gizmo. */
      strcpy(((struct mark_gizmo *)*next_gizmo)->string, record.fields[1]);
      /* Update _next_gizmo to point to the new end of the linked list. */
      next_gizmo = &(*next_gizmo)->next;
      /* Next record. */
      continue;
    }
  }
  /* A body of text must end in a forced break (the sink of the graph). */
  *next_gizmo = xmalloc(sizeof(struct break_gizmo) + 1);
  (*next_gizmo)->type = GIZMO_BREAK;
  (*next_gizmo)->next = NULL;
  ((struct break_gizmo *)*next_gizmo)->spacing = 0;
  ((struct break_gizmo *)*next_gizmo)->force_break = 1;
  ((struct break_gizmo *)*next_gizmo)->selected = 0;
  ((struct break_gizmo *)*next_gizmo)->total_penalty = INT_MAX;
  ((struct break_gizmo *)*next_gizmo)->best_source = NULL;
  ((struct break_gizmo *)*next_gizmo)->style = current_style;
  ((struct break_gizmo *)*next_gizmo)->no_break_width = 0;
  ((struct break_gizmo *)*next_gizmo)->at_break_width = 0;
  ((struct break_gizmo *)*next_gizmo)->no_break
    = ((struct break_gizmo *)*next_gizmo)->at_break
    = ((struct break_gizmo *)*next_gizmo)->strings;
  ((struct break_gizmo *)*next_gizmo)->strings[0] = '\0';
  free_record(&record);
  /* Return the first node in the linked list. */
  return first_gizmo;
}

/* Free the _gizmo_ linked list. */
static void
free_gizmos(struct gizmo *gizmo)
{
  struct gizmo *next;
  /* Loop over each node and free its heap allocated memory. */
  while (gizmo) {
    next = gizmo->next;
    free(gizmo);
    gizmo = next;
  }
}

/* Relax the edges of the _source_ node. */
/* _gizmo_ is the next gizmo after the source node. */
static void
consider_breaks(struct gizmo *gizmo, int source_penalty,
    struct break_gizmo *source, int line_width)
{
  struct break_gizmo *break_gizmo;
  int width, penalty;
  width = 0;
  /* Iterate over gizmos in topological order. */
  for (; gizmo; gizmo = gizmo->next) {
    switch (gizmo->type) {
    case GIZMO_TEXT:
      /* Increment width. */
      width += ((struct text_gizmo *)gizmo)->width;
      break;
    case GIZMO_BREAK:
      break_gizmo = (struct break_gizmo *)gizmo;
      /* If feasible line. */
      if (width + break_gizmo->at_break_width <= line_width) {
        /* Compute edge wight (penalty). */
        penalty = (line_width - width - break_gizmo->at_break_width) / 1000;
        /* Forced breaks have no penalty. */
        if (break_gizmo->force_break)
          penalty = 0;
        /* If this is a new best route to get to the break gizmo. */
        if (break_gizmo->total_penalty > source_penalty + penalty) {
          /* Update best route information. */
          break_gizmo->best_source = source;
          break_gizmo->total_penalty = source_penalty + penalty;
        }
      }
      /* Increment width as if no break occurs. */
      width += break_gizmo->no_break_width;
      /* If all subsequent lines will not be feasible then stop. */
      if (width > line_width)
        goto stop;
      if (width && break_gizmo->force_break)
        goto stop;
      break;
    }
  }
stop:
}

/* Find the optimal breaks in the linked list starting with _gizmo_. */
static void
optimise_breaks(struct gizmo *gizmo, int line_width)
{
  struct break_gizmo *last_break;
  last_break = NULL;
  /* Relax the hypothetical leading break node. */
  consider_breaks(gizmo, 0, NULL, line_width);
  /* Relax every subsequent break node in topological order. */
  for (; gizmo; gizmo = gizmo->next)
    if (gizmo->type == GIZMO_BREAK) {
      last_break = (struct break_gizmo *)gizmo;
      consider_breaks(gizmo->next, last_break->total_penalty, last_break,
          line_width);
    }
  /* Backtrack through the graph to set _selected_ to 1 on all chosen breaks. */
  for (; last_break; last_break = last_break->best_source)
    last_break->selected = 1;
}

/*
 * Add new _string_ with _new_style_ to _buffer_'s _pages_ text mode content.
 * _style_ is updated to match the font state of the text mode content.
 */
static void
print_text(struct dbuffer *buffer, struct style *style,
    const struct style *new_style, const char *string, int *spaces)
{
  /* If string empty the do nothing. */
  if (*string == '\0')
    return;
  /* If a different font is used. */
  if (strcmp(style->font_name, new_style->font_name)
      || style->font_size != new_style->font_size) {
    /* Update style with the new style. */
    memcpy(style, new_style, sizeof(struct style));
    /* Write the new style to the buffer. */
    dbuffer_printf(buffer, "FONT %s %d\n", style->font_name, style->font_size);
  }
  /* Start the string. */
  dbuffer_printf(buffer, "STRING \"");
  /* For each character in the string. */
  while (*string) {
    /* Skip newline characters. */
    if (*string == '\n') {
      string++;
      continue;
    }
    /* Print a backslash before quotation marks or backslashes. */
    if (*string == '"' || *string == '\\')
      dbuffer_putc(buffer, '\\');
    /* Count spaces (used for justified text). */
    if (*string == ' ')
      (*spaces)++;
    /* Add the character to the string. */
    dbuffer_putc(buffer, *string);
    string++;
  }
  /* End the string. */
  dbuffer_printf(buffer, "\"\n");
}

/* Using the _selected_ break gizmos, write _content_ to _output_. */
static void
print_gizmos(FILE *output, struct gizmo *gizmo, int line_width, int align)
{
  int width, height, spaces;
  struct style style;
  struct dbuffer line;
  struct dbuffer line_marks;
  struct text_gizmo *text_gizmo;
  struct break_gizmo *break_gizmo;
  struct mark_gizmo *mark_gizmo;
  width = 0;
  height = 0;
  spaces = 0;
  style.font_name[0] = '\0';
  style.font_size = 0;
  /* _line_ is the text mode _pages_ content for this line. */
  dbuffer_init(&line, 1024 * 4, 1024 * 4);
  /* _line_marks_ are newline separated marks that appear on this line. */
  dbuffer_init(&line_marks, 1024, 1024);
  line_marks.data[0] = '\0';
  /* For each gizmo in the linked list. */
  for (; gizmo; gizmo = gizmo->next) {
    switch (gizmo->type) {
    case GIZMO_TEXT:
      text_gizmo = (struct text_gizmo *)gizmo;
      /* Update line height if this text is taller. */
      if (text_gizmo->style.font_size > height)
        height = text_gizmo->style.font_size;
      /* Increment line width for this text. */
      width += text_gizmo->width;
      /* Add the text to the buffer. */
      print_text(&line, &style, &text_gizmo->style, text_gizmo->string,
          &spaces);
      break;
    case GIZMO_MARK:
      mark_gizmo = (struct mark_gizmo *)gizmo;
      /* Add this mark to the list of marks on this line. */
      dbuffer_printf(&line_marks, "^%s\n", mark_gizmo->string);
      break;
    case GIZMO_BREAK:
      break_gizmo = (struct break_gizmo *)gizmo;
      /* Update line height if the break text is taller. */
      if (break_gizmo->style.font_size > height)
        height = break_gizmo->style.font_size;
      /* If this break was selected. */
      if (break_gizmo->selected) {
        /* Line break occurs here so write and reset the line. */
        width += break_gizmo->at_break_width;
        print_text(&line, &style, &break_gizmo->style, break_gizmo->at_break,
            &spaces);
        /* If line not empty then write the line to _output_. */
        if (line.size) {
          dbuffer_putc(&line, '\0');
          fprintf(output, "box %d\n", height);
          /*
           * Some align modes need the text to be horizontally shifted on the
           * page. To do this, content enters graphic mode with START GRAPHIC
           * and moves to the right with MOVE. Remember to end the graphic
           * mode after exiting text mode.
           */
          if (align == ALIGN_CENTRE) {
            fprintf(output, "START GRAPHIC\n");
            fprintf(output, "MOVE %d 0\n", (line_width - width) / 2000);
          } else if (align == ALIGN_RIGHT) {
            fprintf(output, "START GRAPHIC\n");
            fprintf(output, "MOVE %d 0\n", (line_width - width) / 1000);
          }
          /* Write the actual text. */
          fprintf(output, "START TEXT\n");
          if (align == ALIGN_JUSTIFIED && spaces && !break_gizmo->force_break) {
            /* Justified text increases the size of space characters. */
            fprintf(output, "SPACE %d\n", (line_width - width) / spaces);
          }
          fprintf(output, "%s", line.data);
          /* End text. */
          fprintf(output, "END\n");
          /* End the additional graphic mode. */
          if (align == ALIGN_CENTRE || align == ALIGN_RIGHT) {
            fprintf(output, "END\n");
          }
        }
        /* Write this lines marks. */
        fprintf(output, line_marks.data);
        /* If this break wants spacing, and if its not the last break. */
        if (break_gizmo->spacing
            && (break_gizmo->next == NULL || break_gizmo->next->next))
          fprintf(output, "glue %d\n", break_gizmo->spacing);
        /* Write the _content_ optional page break. */
        fprintf(output, "opt_break\n");
        /* Reset the line. */
        line_marks.size = 0;
        line_marks.data[0] = '\0';
        height = 0;
        width = 0;
        spaces = 0;
        style.font_name[0] = '\0';
        style.font_size = 0;
        line.size = 0;
        line.data[0] = '\0';
      } else {
        /* This break gizmo is not selected. So add the no break string. */
        width += break_gizmo->no_break_width;
        print_text(&line, &style, &break_gizmo->style, break_gizmo->no_break,
            &spaces);
      }
      break;
    }
  }
  dbuffer_free(&line);
  dbuffer_free(&line_marks);
}

static void
die_usage(char *program_name)
{
  /* Print program usage and exit. */
  fprintf(stderr, "Usage: %s -w NUM [-l] [-r] [-j] [-c]\n", program_name);
  exit(1);
}

int
main(int argc, char **argv)
{
  int opt, line_width, align;
  FILE *input_file, *typeface_file, *output_file;
  struct typeface *typeface;
  struct gizmo *gizmos;
  /* Default command line options. */
  line_width = 0;
  align = ALIGN_LEFT;
  /* Parse command line arguments. */
  while ( (opt = getopt(argc, argv, "ljrcw:")) != -1) {
    switch (opt) {
    case 'l':
      align = ALIGN_LEFT;
      break;
    case 'j':
      align = ALIGN_JUSTIFIED;
      break;
    case 'r':
      align = ALIGN_RIGHT;
      break;
    case 'c':
      align = ALIGN_CENTRE;
      break;
    case 'w':
      /* Convert text width to integer. */
      if (str_to_int(optarg, &line_width))
        die_usage(argv[0]);
      /* Convert point space to text space. */
      line_width *= 1000;
      break;
    default:
      /* Unrecognised command line option. */
      die_usage(argv[0]);
    }
  }
  /* Line width must be set by a command line option. */
  if (line_width == 0)
    die_usage(argv[0]);
  input_file = stdin;
  /*
   * Typeface file is hardcoded to be called 'typeface'. This is a design
   * decision: if we could call the typeface file anything we wanted, then
   * other programs part of the typesetting process would struggle to locate
   * it. Just like 'Makefile', 'typeface' is a hard-coded file name.
   */
  typeface_file = fopen("typeface", "r");
  if (typeface_file == NULL) {
    fprintf(stderr, "Failed to open typeface file.\n");
    return 1;
  }
  output_file = stdout;
  /* Parse the typeface file. */
  typeface = open_typeface(typeface_file);
  fclose(typeface_file);
  /* Parse gizmos from standard input. */
  gizmos = parse_gizmos(input_file, typeface);
  /* Select the optimal line breaks. */
  optimise_breaks(gizmos, line_width);
  /* Print the _content_ to standard output. */
  print_gizmos(output_file, gizmos, line_width, align);
  /* Free memory and close files. */
  free_typeface(typeface);
  free_gizmos(gizmos);
  fclose(input_file);
  fclose(output_file);
  return 0;
}
