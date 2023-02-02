#include "common.hh"
#include "verilated.h"
#include "axi.hh"
#include "Vmycpu_top.h"
#include <csignal>
#include <cstdio>
#include "dpic.hh"
#include "difftest-dut.hh"
#include "difftest-ref.hh"
#ifdef CONFIG_TRACE_ON
#include __WAVE_INC__
#endif

using namespace std;
sim_status_t sim_status = SIM_STOP;
FILE* golden_trace = nullptr;
const word_t end_pc = 0xbfc00100;
uint64_t inst_count = 0;

void compare (debug_info_t debug){
    if (debug.pc==end_pc){
        __ASSERT_SIM__(0, "run to the end pc");
    }
    if (debug.wen != 0 && debug.wnum != 0) {
        debug_info_t ref;
        unsigned int check;
        unsigned int wnum;
        int res = fscanf(golden_trace, "%u %x %x %x",&check,&ref.pc,&wnum,&ref.wdata);
        ref.wnum = wnum;
        inst_count++;
        if ((debug.pc != ref.pc) || (debug.wnum != ref.wnum) || (debug.wdata != ref.wdata)) {
            __ASSERT_SIM__(((debug.pc == ref.pc) && (debug.wnum == ref.wnum) && (debug.wdata == ref.wdata)), \
                    "Error!!!\n"  \
                    "    mycpu    : PC = 0x%8x, wb_wnum = 0x%2x, wb_wdata = 0x%8x\n"  \
                    "    reference: PC = 0x%8x, wb_wnum = 0x%2x, wb_wdata = 0x%8x\n", \
                    debug.pc, debug.wnum, debug.wdata, ref.pc, ref.wnum, ref.wdata
                    );
        }
    }
}
int main (int argc, char *argv[]) {
    Verilated::commandArgs(argc, argv);
    std::signal(SIGINT, [](int) {sim_status = SIM_ABORT;});
    Verilated::traceEverOn(CONFIG_TRACE_ON);

    unique_ptr<Vmycpu_top> top(new Vmycpu_top());
    unique_ptr<axi_paddr> axi(new axi_paddr(top.get()));

    AddrIntv inst_range = AddrIntv(0x1fc00000,(uint8_t)22);
    AddrIntv confreg_range = AddrIntv(0x1faf0000,(uint8_t)16);

    unique_ptr<Pmem> v_inst_mem (new Pmem(inst_range));
    v_inst_mem->load_binary(0,__FUNC_BIN__);
    axi->paddr_top.add_dev(inst_range, v_inst_mem.get());
    unique_ptr<PaddrConfreg> v_confreg (new PaddrConfreg(false));
    axi->paddr_top.add_dev(confreg_range, v_confreg.get());

    PaddrTop *nemu_paddr_top = new PaddrTop();
    unique_ptr<Pmem> nemu_inst_mem (new Pmem(inst_range));
    v_inst_mem->load_binary(0,__FUNC_BIN__);
    nemu_paddr_top->add_dev(inst_range, nemu_inst_mem.get());
    unique_ptr<PaddrConfreg> nemu_confreg (new PaddrConfreg(false));
    nemu_paddr_top->add_dev(confreg_range, nemu_confreg.get());

    ref_init((void*)nemu_paddr_top);

    wave_file_t tfp;
    IFDEF(CONFIG_TRACE_ON,top->trace(&tfp,0));
    uint64_t ticks = 0;

    bool stop = false;
    v_confreg->set_switch(0);
    top->aresetn = 0;
    IFDEF(CONFIG_TRACE_ON,tfp.open(__WAVE_DIR__ "func_test." CONFIG_WAVE_EXT));
    uint64_t rst_ticks = 5;
    uint64_t last_commit = ticks;
    uint64_t commit_timeout = 1024;
    top->aclk = 0;
    sim_status = SIM_RUN;
    uint64_t last_commit_time = ticks;
    while (!Verilated::gotFinish() && !stop) {
        if (rst_ticks  > 0) {
            top->aresetn = 0;
            rst_ticks --;
            axi->reset();
        }
        else top->aresetn = 1;
        top->aclk = !top->aclk;
        bool valid_posedge = top->aclk && top->aresetn;
        if (valid_posedge) {
            axi->calculate_output();
            top->eval();
            v_confreg->tick();
            axi->update_output();
            IFDEF(CONFIG_TRACE_ON,tfp.dump(ticks));
            stop = sim_status!=SIM_RUN;
            if (sim_status!=SIM_RUN) break;
            ref_tick_int(0);
            uint8_t commit_num =dpi_retire();
            if (commit_num>0){
                for (size_t i = 0; i < commit_num; i++) {
                    ref_exec_once(false);
                }
                CPU_state mycpu, ref_r;
                dut_get_status(&mycpu);
                ref_get_status(&ref_r);
                bool res = difftest_check(&mycpu, &ref_r);
                if (!res){
                    __ASSERT_SIM__(0, "DIFFTEST ERROR!")
                        difftest_show_error(&mycpu, &ref_r);
                }
            }
        }
        else {
            top->eval();
            IFDEF(CONFIG_TRACE_ON,tfp.dump(ticks));
        }
        ticks ++;
    }
    fclose(golden_trace);
    IFDEF(CONFIG_TRACE_ON,tfp.close());
    top->final();
    printf("total ticks = %lu\n", ticks);
    return 0;
}
