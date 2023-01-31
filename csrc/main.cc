#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vmycpu_top.h"
#include <csignal>
#include "axi.hh"
#include "dpic.hh"

using namespace std;
sim_status_t sim_status = SIM_STOP;
FILE* golden_trace;

void compare (debug_info_t debug){
    if (debug.wen != 0 && debug.wnum != 0) {
        debug_info_t ref;
        unsigned int check;
        fscanf(golden_trace, "%u %x %s %x",&check,&debug.pc,&debug.wnum,&debug.wdata);
        if ((debug.pc != ref.pc) || (debug.wnum != ref.wnum) || (debug.wdata != ref.wdata)) {
            __ASSERT_SIM__(((debug.pc == ref.pc) && (debug.wnum == ref.wnum) && (debug.wdata == ref.wdata)), \
                        "Error!!!\n"  \
            "    reference: PC = 0x%8x, wb_wnum = 0x%2x, wb_wdata = 0x%8x\n"  \
            "    mycpu    : PC = 0x%8x, wb_wnum = 0x%2x, wb_wdata = 0x%8x\n", \
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

    Pmem v_inst_mem = Pmem(inst_range);
    v_inst_mem.load_binary(0x1fc00000,  "../nscscc-group/func_test_v0.01/soft/func/obj/main.bin");
    axi->paddr_top.add_dev(inst_range, &v_inst_mem);

    PaddrConfreg v_confreg = PaddrConfreg(false);
    axi->paddr_top.add_dev(confreg_range, &v_confreg);

    golden_trace = fopen("../nscscc-group/func_test_v0.01/cpu132_gettrace/test.txt","r");

    VerilatedVcdC vcd;
    IFDEF(CONFIG_TRACE_ON,top->trace(&vcd,0));
    uint64_t ticks = 0;

    bool stop = false;
    v_confreg.set_switch(0);
    top->aresetn = 0;
    IFDEF(CONFIG_TRACE_ON,vcd.open("func_test.vcd"));
    uint64_t rst_ticks = 5;
    uint64_t last_commit = ticks;
    uint64_t commit_timeout = 1024;
    top->aclk = 0;
    while (!Verilated::gotFinish()) {
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
            v_confreg.tick();
            axi->update_output();
            debug_info_t debug0, debug1;
            dpi_get_debug_info0(debug0);
            dpi_get_debug_info1(debug1);
        }
        else {
            top->eval();
        }
        vcd.dump(ticks);
        IFDEF(CONFIG_TRACE_ON,vcd.dump(ticks));
        ticks ++;
    }
    IFDEF(CONFIG_TRACE_ON,vcd.close());
    top->final();
    printf("total ticks = %lu\n", ticks);
    return 0;
}
