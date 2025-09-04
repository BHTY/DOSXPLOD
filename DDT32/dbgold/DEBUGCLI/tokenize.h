#pragma once
#include <stdint.h>
#include <stddef.h>

/* Definitions for tokenizer */

#define TOK_ID 0
#define TOK_OP 1
#define TOK_STR 2

#define IS_OPERATOR(ch)         ((ch) == ':' || (ch) == '!' || (ch) == '(' || (ch) == ')' || (ch) == '[' || (ch) == ']' || (ch) == '+' || (ch) == '-' || (ch) == '*' || (ch) == '<' || (ch) == '>' || (ch) == ',' || (ch) == '@' || (ch) == '&' || (ch) == '%' || (ch) == '|' || (ch) == '~' || (ch) == '^')
#define IS_OP_EXPR(ch)          ((ch) == '+' || (ch) == '-' || (ch) == '*' || (ch) == '<' || (ch) == '>' || (ch) == '&' || (ch) == '|' || (ch) == '~' || (ch) == '^')

typedef struct _STRING_VIEW {
    char* start;
    size_t len;
} STRING_VIEW;

typedef struct _TOKEN {
    STRING_VIEW str;
    int type;
} TOKEN;

typedef struct _TOKENIZED {
    int nMaxTokens;
    int nTokens;
    TOKEN tokens[1];
} TOKENIZED;

typedef struct _TOKEN_VIEW {
    int start, len;
    TOKENIZED* toks;
} TOKEN_VIEW;

int dump_token(char* start, size_t len, int type, TOKENIZED* toks);
int tokenize(char* str, TOKENIZED* toks);
TOKENIZED* alloc_tokenized(size_t tokens);
void print_tokenized(TOKENIZED* tokenized);
