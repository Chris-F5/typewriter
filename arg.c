/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "arg.h"

int opt_arg_int;
const char *opt_arg_string;

static int opt_index = 1;

static int
is_letter(char c)
{
  unsigned char u;
  u = (unsigned char)c;
  return (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z');
}

int
next_opt(int argc, char **argv, const char *opt_string)
{
  const char *opt;
  char *endptr;

  if (opt_index == argc)
    return -1;
  if (argv[opt_index][0] != '-') {
    opt_arg_string = argv[opt_index];
    opt_index += 1;
    return 0;
  }
  opt = argv[opt_index] + 1;
  while (*opt_string) {
    if (is_letter(*opt_string) && opt[0] == *opt_string)
      break;
    opt_string++;
  }
  if (*opt_string == '\0') {
    fprintf(stderr, "Invalid option %s.\n", argv[opt_index]);
    exit(1);
  }
  if (opt_string[1] == '\0' || is_letter(opt_string[1])) {
    opt_index += 1;
    return opt_string[0];
  }
  if (opt[1] == '\0')
    opt_arg_string = argv[++opt_index];
  else
    opt_arg_string = opt + 1;
  if (opt_arg_string == NULL || opt_arg_string[0] == '\0') {
    fprintf(stderr, "Option -%c needs argument.\n", *opt_string);
    exit(1);
  }
  switch (opt_string[1]) {
  case '*':
    opt_arg_int = 0;
    break;
  case '#':
    opt_arg_int = strtol(opt_arg_string, &endptr, 10);
    if (*endptr != '\0') {
      fprintf(stderr, "Option -%c expects integer argument.\n", *opt_string);
      exit(1);
    }
    break;
  default:
    fprintf(stderr, "Invalid opt_string argument type %c.\n", opt_string[1]);
    exit(1);
  }
  opt_index++;
  return *opt_string;
}
