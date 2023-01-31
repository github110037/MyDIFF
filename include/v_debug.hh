#ifndef __V_DEBUG__
#define __V_DEBUG__
#include "common.hh"
typedef struct{
    word_t pc;
    uint8_t wen;
    uint8_t wnum;
    word_t wdata;
} debug_info_t;
#endif // !__V_DEBUG__
