#ifndef __DIFFTEST_HPP__
#define __DIFFTEST_HPP__
#include <cstdint>
typedef struct {
    uint32_t gpr[32];
    uint32_t pc;
} CPU_state ;
uint8_t vtop_retire();
void vtop_getState(CPU_state *mycpu);
void print_diff(CPU_state *cpu, CPU_state *ref_r);
bool check_state(CPU_state *cpu, CPU_state *ref_r);
#endif
