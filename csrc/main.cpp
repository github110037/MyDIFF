#include "common.hh"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vmycpu_top.h"
#include <csignal>
#include <cstdio>
#include "axi.hh"
#include "dpic.hh"

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
        // printf("now inst_count:\t%lu\n",inst_count);
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
    v_inst_mem->load_binary(0,  "../nscscc-group/func_test_v0.01/soft/func/obj/main.bin");
    axi->paddr_top.add_dev(inst_range, v_inst_mem.get());

    unique_ptr<PaddrConfreg> v_confreg (new PaddrConfreg(false));
    axi->paddr_top.add_dev(confreg_range, v_confreg.get());

    golden_trace = fopen("../nscscc-group/func_test_v0.01/cpu132_gettrace/golden_trace.txt","r+");
    if (golden_trace==nullptr){
        Log("can not open golden_trace");
    }

    VerilatedVcdC vcd;
    IFDEF(CONFIG_TRACE_ON,top->trace(&vcd,0));
    uint64_t ticks = 0;

    bool stop = false;
    v_confreg->set_switch(0);
    top->aresetn = 0;
    IFDEF(CONFIG_TRACE_ON,vcd.open("func_test.vcd"));
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
            stop = sim_status!=SIM_RUN;
            axi->calculate_output();
            top->eval();
            v_confreg->tick();
            axi->update_output();
            debug_info_t debug0, debug1;
            dpi_get_debug_info0(debug0);
            dpi_get_debug_info1(debug1);
            compare(debug0);
            compare(debug1);
            int c_num = dpi_retire();
            // if (c_num>0){
            //     last_commit_time = ticks;
            // }
            // if (ticks - last_commit_time > commit_timeout){
            //     assert(0);
            // }
        }
        else {
            top->eval();
        }
        IFDEF(CONFIG_TRACE_ON,vcd.dump(ticks));
        ticks ++;
    }
    fclose(golden_trace);
    IFDEF(CONFIG_TRACE_ON,vcd.close());
    top->final();
    printf("total ticks = %lu\n", ticks);
    return 0;
}