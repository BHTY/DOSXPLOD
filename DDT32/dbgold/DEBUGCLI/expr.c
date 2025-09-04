#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "tokenize.h"
#include "expr.h"

#define UNKNOWN 0
#define AWAITING_OPERAND 1
#define AWAITING_OPERATOR 2
#define AWAITING_PAREN 3

/* Find the end of the expr */
uint32_t find_expr(TOKEN_VIEW* view) {
    int i; 
    char ch;
    int paren_depth = 0;

    if (view->start >= view->toks->nTokens) return -1;

    i = view->start + 1;
    ch = view->toks->tokens[view->start].str.start[0];

    int mode = AWAITING_OPERATOR;

    if (ch == '(') {
        paren_depth = 1;
        mode = AWAITING_PAREN;
    }
    else if (ch == '~' || ch == '-') {
        mode = AWAITING_OPERAND;
    }

    for (; i < view->len + view->start; i++) {
        ch = view->toks->tokens[i].str.start[0];

        switch (mode) {
        case AWAITING_OPERAND:
            if (ch == '~' || ch == '-') {
                mode = AWAITING_OPERAND;
            }
            else if (ch == '(') {
                mode = AWAITING_PAREN;
                paren_depth = 1;
            }
            else {
                mode = AWAITING_OPERATOR;
            }

            break;
        case AWAITING_OPERATOR:
            if (IS_OP_EXPR(ch)) {
                mode = AWAITING_OPERAND;
            }
            else {
                return i; /* We've reached the end */
            }

            break;
        case AWAITING_PAREN:
            if (ch == '(') paren_depth++;
            if (ch == ')') {
                paren_depth--;

                if (paren_depth == 0) {
                    mode = AWAITING_OPERATOR;
                }
            }
            break;
        default: break;
        }
    }

    return i;
}

/*
    C order of operations

1   Parens      ( )             7
2   NOT/Neg     (~/-)           6
3   Multiply    (*)             5
4   Add Sub     (+ -)           4
5   L/R Shift   (<< >>)         3
6   Bitwise AND (&)             2
7   Bitwise XOR (^)             1
8   Bitwise OR  (|)             0

*/

char* cnt_names[] = { "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI", "BX", "BP", "SI", "DI" };
char* reg_names[] = { "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI", "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI", "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" };

int get_reg_id(char* str) {
    int i = 0;

    for (i = 0; i < ARRAYSIZEOF(cnt_names); i++) {
        if (stricmp(str, cnt_names[i]) == 0) {
            return i;
        }
    }

    return -1;
}

int is_reg(char* str) {
    int i = 0;

    for (i = 0; i < ARRAYSIZEOF(reg_names); i++) {
        if (stricmp(str, reg_names[i]) == 0) return 1;
    }

    return 0;
}

#define copy_str(d, s)              memcpy((d), (s).start, (s).len); (d)[(s).len] = 0;

int id_exists(char* str) {
    return 0;
}

uint32_t id_value(char* str) {
    return 0;
}

// mode: 0 = normal parsing, 1 = memory address
// returns: value in *value, status in return value
// 0: Normal
// 1: Register
// 2: Parsing failed
uint32_t parse_expr(TOKEN_VIEW* view, uint32_t* value, int mode, reg_cnt* cnt) {
    int op_level;
    int paren_level = 0;

    uint32_t left_return, right_return;
    uint32_t left_value, right_value;

    TOKEN_VIEW tvl, tvr;

    char buf[40];

    /*
    for (int i = 0; i < view->len; i++) {
        printf("%c ", view->toks->tokens[view->start + i].str.start[0]);
    }
    printf(" (%d)\n", view->len);
    */

    if (view->len == 0) return 2;

    if (view->len == 1) { /* Just parsing a single token */
        copy_str(buf, view->toks->tokens[view->start].str);

        if (is_reg(buf)) { // is an register
            if (mode) { /* parsing a memory address for x86 purposes */
                *value = get_reg_id(buf); /* get reg ID */
                return 1;
            }
            else { /* just getting the value of the register */
                *value = 0; /* get reg value */
                return 0;
            }
        }
        else if (id_exists(buf)) { // is an identifier
            *value = id_value(buf);
            return 0;
        }
        else if (buf[0] == '\'' && buf[2] == '\'') { // is a character constant
            *value = buf[1];
            return 0;
        }
        else { // is a number
            uint32_t num;

            if (sscanf(buf, "%x", &num)) {
                *value = num;
                return 0;
            }
            else {
                return 2; /* Parsing failed - bad identifier */
            }
        }
    }

    /* Eliminate leading paren */
    if (view->toks->tokens[view->start].str.start[0] == '(') {
        int i;

        for (i = view->start; i < view->start + view->len; i++) {
            if (view->toks->tokens[i].str.start[0] == '(') {
                paren_level++;
            }
            else if (view->toks->tokens[i].str.start[0] == ')') {
                paren_level--;
            }

            if (paren_level == 0) {
                break;
            }
        }

        if (paren_level != 0) { /* failed to find matching (closing) paren */
            return 2;
        }
        else {
            if (i == view->start + view->len - 1) { /* the entire view is a paren */
                tvl.len = view->len - 2;
                tvl.start = view->start + 1;
                tvl.toks = view->toks;

                left_return = parse_expr(&tvl, &left_value, mode, cnt);

                *value = left_value;
                return left_return;
            }
        }
    }

    /* Remove operators */
    for (op_level = 0; op_level < 8; op_level++) {
        int i;
        int paren_level = 0;

        for (i = view->start; i < view->start + view->len; i++) {
            if (view->toks->tokens[i].str.start[0] == '(') {
                paren_level++;
            }
            else if (view->toks->tokens[i].str.start[0] == ')') {
                paren_level--;
            }
            else if (paren_level == 0) {
                tvl.toks = view->toks;
                tvl.start = view->start;
                tvl.len = i - view->start;

                tvr.toks = view->toks;
                tvr.start = i + 1;
                tvr.len = view->start + view->len - (i + 1);

                switch (op_level) {
                case 0:
                    if (view->toks->tokens[i].str.start[0] == '|') {
                        if (tvl.len == 0 || tvr.len == 0) return 2;

                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2 || (left_return == 1 && mode == 1)) { /* either parsing failed or we tried to OR a register to calc a memory addr */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2 || (right_return == 1 && mode == 1)) { /* either parsing failed or we tried to OR a register to calc a memory addr */
                            return 2;
                        }

                        *value = left_value | right_value;

                        return 0;
                    }
                    break;

                case 1:
                    if (view->toks->tokens[i].str.start[0] == '^') {
                        if (tvl.len == 0 || tvr.len == 0) return 2;

                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2 || (left_return == 1 && mode == 1)) { /* either parsing failed or we tried to XOR a register to calc a memory addr */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2 || (right_return == 1 && mode == 1)) { /* either parsing failed or we tried to XOR a register to calc a memory addr */
                            return 2;
                        }

                        *value = left_value ^ right_value;
                        return 0;
                    }
                    break;

                case 2:
                    if (view->toks->tokens[i].str.start[0] == '&') {
                        if (tvl.len == 0 || tvr.len == 0) return 2;

                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2 || (left_return == 1 && mode == 1)) { /* either parsing failed or we tried to AND a register to calc a memory addr */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2 || (right_return == 1 && mode == 1)) { /* either parsing failed or we tried to AND a register to calc a memory addr */
                            return 2;
                        }

                        *value = left_value & right_value;
                        return 0;
                    }
                    break;

                case 3:
                    if (view->toks->tokens[i].str.start[0] == '<' || view->toks->tokens[i].str.start[0] == '>') {
                        if (tvl.len == 0 || tvr.len == 0) return 2;

                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2 || (left_return == 1 && mode == 1)) { /* either parsing failed or we tried to shift a register to calc a memory addr */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2 || (right_return == 1 && mode == 1)) { /* either parsing failed or we tried to shift a register to calc a memory addr */
                            return 2;
                        }

                        if (view->toks->tokens[i].str.start[0] == '<') {
                            *value = left_value << right_value;
                        }
                        else {
                            *value = left_value >> right_value;
                        }

                        return 0;
                    }
                    break;

                case 4:
                    if ((view->toks->tokens[i].str.start[0] == '+') && ((tvl.len > 0) && (tvr.len > 0))) {

                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2) { /* parsing failed */
                            continue;
                        }

                        if (left_return >= 2) { /* parsing failure */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2) { /* either parsing failed */
                            return 2;
                        }

                        if (mode == 1 && (right_return == 1 || left_return == 1)) {
                            uint32_t total = 0;

                            if ((right_return == 1 && left_return == 1)) { /* adding two registers */
                                if (left_value == -1 || right_value == -1) { /* bad register for indexing */
                                    return 2;
                                }

                                cnt->regs[left_value]++;
                                cnt->regs[right_value]++;
                            }
                            else if (right_return == 1) { /* const + reg */
                                if (right_value == -1) { /* bad register for indexing */
                                    return 2;
                                }

                                total = left_value;
                                cnt->regs[right_value]++;
                            }
                            else if (left_return == 1) { /* reg + const */
                                if (left_value == -1) { /* bad register for indexing */
                                    return 2;
                                }

                                total = right_value;
                                cnt->regs[left_value]++;
                            }

                            *value = total;

                            return 0;
                        }

                        *value = left_value + right_value;

                        return 0;
                    }
                    else if ((view->toks->tokens[i].str.start[0] == '-') && ((tvl.len > 0) && (tvr.len > 0))) {
                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2) { /* parsing failed */
                            continue;
                        }

                        if (left_return >= 2 || (left_return == 1 && mode == 1)) { /* we tried to addsub a register to calc a memory addr */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2 || (right_return == 1 && mode == 1)) { /* either parsing failed or we tried to addsub a register to calc a memory addr */
                            return 2;
                        }

                        *value = left_value - right_value;

                        return 0;
                    }
                    break;

                case 5:
                    if (view->toks->tokens[i].str.start[0] == '*') {
                        if (tvl.len == 0 || tvr.len == 0) return 2;

                        left_return = parse_expr(&tvl, &left_value, mode, cnt);

                        if (left_return >= 2) { /* either parsing failed  */
                            return 2;
                        }

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2) { /* either parsing failed */
                            return 2;
                        }

                        if (mode == 1) {
                            if ((right_return == 1 && left_return == 1)) { /* if we're calculating a memory address, we can't be multiplying two regs */
                                return 2;
                            }
                            if (right_return == 1) {
                                if (right_value == -1) { /* bad register to use for indexing */
                                    return 2;
                                }

                                cnt->regs[right_value] += left_value;

                                *value = 0;
                                return 0;
                            }
                            else if (left_return == 1) {
                                if (left_value == -1) { /* bad register to use for indexing */
                                    return 2;
                                }

                                cnt->regs[left_value] += right_value;

                                *value = 0;
                                return 0;
                            }
                        }

                        *value = left_value * right_value;
                        return 0;
                    }
                    break;

                case 6:
                    if ((view->toks->tokens[i].str.start[0] == '~' || view->toks->tokens[i].str.start[0] == '-') && ((tvl.len == 0))) {
                        if (tvr.len == 0) return 2;

                        right_return = parse_expr(&tvr, &right_value, mode, cnt);

                        if (right_return >= 2 || (right_return == 1 && mode == 1)) { /* either parsing failed or we tried to NOT/negate a register to calc a memory addr */
                            return 2;
                        }

                        if (view->toks->tokens[i].str.start[0] == '~') {
                            *value = ~right_value;
                        }
                        else {
                            *value = -right_value;
                        }

                        return 0;
                    }
                    break;
                }
            }
        }

        if (paren_level > 0) { /* failed to find balancing closing paren */
            return 2;
        }
    }

    return 2;
}