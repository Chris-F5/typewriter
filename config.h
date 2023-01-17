enum grammar_rules {
  GRAMMAR_ROOT,
  GRAMMAR_WORD,
};

const int * const grammar[] = {
  [GRAMMAR_ROOT] = (int []) {
    PARSE_SEQ,
      PARSE_ANY, PARSE_CHAR, '\n',
      PARSE_ANY, PARSE_SEQ,
        PARSE_GRAMMAR, GRAMMAR_WORD,
        PARSE_SOME, PARSE_CHAR, '\n',
      END_PARSE,
    END_PARSE,
  },
  [GRAMMAR_WORD] = (int []) {
    PARSE_SYMBOL_STRING, SYMBOL_WORD, PARSE_SOME, PARSE_CHAR_RANGE, '!', '~',
  },
};
