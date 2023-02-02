#ifndef __DIFFTEST_HPP__
#define __DIFFTEST_HPP__
#include <cstdint>
typedef struct {
    uint32_t gpr[32];
    uint32_t pc;
} CPU_state ;
uint8_t vtop_retire();
void dut_get_status(CPU_state *mycpu);
void difftest_show_error(CPU_state *cpu, CPU_state *ref_r);
bool difftest_check(CPU_state *cpu, CPU_state *ref_r);
#endif
