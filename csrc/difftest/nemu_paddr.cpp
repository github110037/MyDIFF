#include "common.hh"
#include "PaddrInterface.h"
#include "PaddrInterface.hh"
bool paddr_do_read (void* paddr_top, word_t addr, size_wstrb info, word_t* data){
    PaddrTop* obj = (PaddrTop*) paddr_top;
    return obj->do_read(addr, info, data);
}
bool paddr_do_write(void* paddr_top, word_t addr, size_wstrb info, const word_t data){
    PaddrTop* obj = (PaddrTop*) paddr_top;
    return obj->do_write(addr, info, data);
}
