#ifndef __MMIO_DEV__
#define __MMIO_DEV__
#include <cassert>
#include <stdint.h>
#include "common.hh"
// address interval
class AddrIntv {
    public:
    // example: [0x100,0x1ff]
    word_t start;   // include:0x01000
    word_t mask;
    AddrIntv(word_t _start, word_t _len): start(_start), mask(_len - 1){
        assert((start & mask) == 0);
    };
    AddrIntv(word_t _start, uint8_t mask_bits){
        AddrIntv(_start,(word_t)BITMASK(mask_bits));
    }
    word_t end(){
        return start + mask;
    }
};
typedef struct{
    uint8_t size: 4;
    uint8_t wstrb:4;
} size_wstrb;
class PaddrInterface {
    public:
        virtual bool do_read (word_t addr, size_wstrb info, word_t* data) = 0;
        virtual bool do_write(word_t addr, size_wstrb info, const word_t data) = 0;
};

#endif
