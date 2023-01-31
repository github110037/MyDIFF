#ifndef __COMMON_HH__
#define __COMMON_HH__
#include "macro.hh"
#include <stdint.h>
#include <cinttypes>
typedef MUXDEF(CONFIG_ISA64, uint64_t, uint32_t) word_t;
typedef MUXDEF(CONFIG_ISA64, int64_t, int32_t)  sword_t;
#define FMT_WORD MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32)
#define FMT_WORD_X MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32)
#define FMT_WORD_U MUXDEF(CONFIG_ISA64, "%20" PRIu64, "%12" PRIu32)
#define FMT_WORD_D MUXDEF(CONFIG_ISA64, "%20" PRId64, "%12" PRId32)

typedef word_t vaddr_t;
typedef MUXDEF(PMEM64, uint64_t, uint32_t) paddr_t;
#define FMT_PADDR MUXDEF(PMEM64, "0x%016" PRIx64, "0x%08" PRIx32)
typedef uint16_t ioaddr_t;
typedef enum{
    SIM_RUN,    // still run
    SIM_STOP,   // gdb breakpoint or watchpoint
    SIM_ABORT,  // difftest error or mem check error
    SIM_END,    // finish run sim
    SIM_QUIT,   // wait to static
}sim_status_t;
extern sim_status_t sim_status;
#define __ASSERT_SIM__(cond,fmt,...) if (!(cond)) {\
    Log(fmt, ## __VA_ARGS__);\
    sim_status = SIM_ABORT;}
#endif
