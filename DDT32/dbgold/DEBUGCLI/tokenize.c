#include "tokenize.h"
#include <stdlib.h>
#include <string.h>

int dump_token(char* start, size_t len, int type, TOKENIZED* toks) {
    TOKEN* token;

    if (toks->nTokens == toks->nMaxTokens) return 1; /* Too many tokens! */

    token = &toks->tokens[toks->nTokens++];

    token->type = type;
    token->str.start = start;
    token->str.len = len;

    return 0;
}

/**
* Tokens:
* Identifiers (names/numbers)
* Strings
*
* Delimiters:
* - Spaces (when not enclosed between quotes)
* - Close quote
* - The following : ! ( ) [ ] + - * << >> , @ % & |
* */

int tokenize(char* str, TOKENIZED* toks) {
    char* curtok_start = str;
    size_t curtok_len = 0;
    int curtok_type;
    int inside_token = 0;


    while (*str && *str != '\n') {
        /* Find the first character that isn't a space and determine the type of the first token */
        if (inside_token == 0) {
            while (*(str) == ' ') { str++; }
            inside_token = 1;
            curtok_start = str;
            curtok_len = 0;

            if (*str == '"') {
                curtok_type = TOK_STR;
            }
            else if (IS_OPERATOR(*str)) {
                curtok_type = TOK_OP;
            }
            else if (*str == 0 || *str == '\n') { /* BUGBUG: All-spaces */
                break;
            }
            else {
                curtok_type = TOK_ID;
            }

            /*printf("Starting token of type %d (first char = %c)\n", curtok_type, *str);*/
        }

        switch (curtok_type) {
        case TOK_ID:
            if (*str == ' ' || *str == '"' || IS_OPERATOR(*str)) {
                if (dump_token(curtok_start, curtok_len, curtok_type, toks)) return 1; /* Too many tokens! */
                inside_token = 0;
            }
            else {
                curtok_len++;
                str++;
            }
            break;
        case TOK_STR:
            curtok_len++;

            if (*str == '"' && curtok_len != 1) {
                if (dump_token(curtok_start, curtok_len, curtok_type, toks)) return 1; /* Too many tokens! */
                inside_token = 0;
            }

            str++;

            break;
        case TOK_OP:
            //if (!IS_OPERATOR(*str)) {
            //    if (dump_token(curtok_start, curtok_len, curtok_type, toks)) return 1; /* Too many tokens! */
            //    inside_token = 0;
            //}
            //else {
            //    curtok_len++;
            //    str++;
            //}

            if (dump_token(curtok_start, 1, curtok_type, toks)) return 1; /* Too many tokens! */
            inside_token = 0;
            str++;

            break;
        default:
            return 1;
            break;
        }

    }

    if (inside_token && curtok_len) {
        if (dump_token(curtok_start, curtok_len, curtok_type, toks)) return 1; /* Too many tokens! */
    }

    return 0;
}

TOKENIZED* alloc_tokenized(size_t tokens) {
    TOKENIZED* tokenized = (TOKENIZED*)malloc(sizeof(TOKENIZED) + (tokens - 1) * sizeof(TOKEN));

    if (tokenized == NULL) return tokenized;

    memset(tokenized, 0, sizeof(TOKENIZED) + (tokens - 1) * sizeof(TOKEN));

    tokenized->nMaxTokens = tokens;
    tokenized->nTokens = 0;

    return tokenized;
}

void print_tokenized(TOKENIZED* tokenized) {
    int i;

    printf("TOKENS: %d/%d\n", tokenized->nTokens, tokenized->nMaxTokens);

    for (i = 0; i < tokenized->nTokens; i++) {
        int p;

        printf("[%2d] %d: ", i, tokenized->tokens[i].type);

        for (p = 0; p < tokenized->tokens[i].str.len; p++) {
            printf("%c", tokenized->tokens[i].str.start[p]);
        }

        printf("\n");
    }
}
