#ifndef __PADDR_IF_H__
#define __PADDR_IF_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "size_wstrb.hh"
bool paddr_do_read (void* paddr_top, word_t addr, size_wstrb info, word_t* data);
bool paddr_do_write(void* paddr_top, word_t addr, size_wstrb info, const word_t data);

#ifdef __cplusplus
}
#endif

#endif /* __MATHER_H__ */
