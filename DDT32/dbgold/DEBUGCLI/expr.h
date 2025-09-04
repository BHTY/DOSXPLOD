#pragma once
#include <stdint.h>

#define ARRAYSIZEOF(a)          (sizeof(a)/sizeof(a[0]))

typedef struct reg_cnt {
    int regs[12];
} reg_cnt;

uint32_t find_expr(TOKEN_VIEW* view);
uint32_t parse_expr(TOKEN_VIEW* view, uint32_t* value, int mode, reg_cnt* cnt);
