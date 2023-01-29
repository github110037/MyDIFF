#include <cstdint>
#include <cstdio>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <thread>
#include <csignal>
#include <sstream>

#include "macro.hpp"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vmycpu_top__Dpi.h"
#include "Vmycpu_top.h"

#include "dpic.hpp"
#include "difftest.hpp"
#include "axi.hpp"
#include "device/uart8250.hpp"
#include "device/mmio_mem.hpp"
#include "device/nscscc_confreg.hpp"
#include "core/mips/mips_core.hpp"

void connect_wire(axi4_ptr &mmio_ptr, Vmycpu_top *top) {/*{{{*/
    // connect
    // mmio
    // aw   
    mmio_ptr.awaddr     = &(top->awaddr);
    mmio_ptr.awburst    = &(top->awburst);
    mmio_ptr.awid       = &(top->awid);
    mmio_ptr.awlen      = &(top->awlen);
    mmio_ptr.awready    = &(top->awready);
    mmio_ptr.awsize     = &(top->awsize);
    mmio_ptr.awvalid    = &(top->awvalid);
    // w
    mmio_ptr.wdata      = &(top->wdata);
    mmio_ptr.wlast      = &(top->wlast);
    mmio_ptr.wready     = &(top->wready);
    mmio_ptr.wstrb      = &(top->wstrb);
    mmio_ptr.wvalid     = &(top->wvalid);
    // b
    mmio_ptr.bid        = &(top->bid);
    mmio_ptr.bready     = &(top->bready);
    mmio_ptr.bresp      = &(top->bresp);
    mmio_ptr.bvalid     = &(top->bvalid);
    // ar
    mmio_ptr.araddr     = &(top->araddr);
    mmio_ptr.arburst    = &(top->arburst);
    mmio_ptr.arid       = &(top->arid);
    mmio_ptr.arlen      = &(top->arlen);
    mmio_ptr.arready    = &(top->arready);
    mmio_ptr.arsize     = &(top->arsize);
    mmio_ptr.arvalid    = &(top->arvalid);
    // r
    mmio_ptr.rdata      = &(top->rdata);
    mmio_ptr.rid        = &(top->rid);
    mmio_ptr.rlast      = &(top->rlast);
    mmio_ptr.rready     = &(top->rready);
    mmio_ptr.rresp      = &(top->rresp);
    mmio_ptr.rvalid     = &(top->rvalid);
}/*}}}*/

bool running = true;
bool confreg_uart = true;
bool perf_stat = false;
bool diff_uart = true;
bool trace_start = false;
long trace_start_time = 0;

unsigned int *pc;
void write_tcl_marker(uint64_t error_time){
    using namespace std;
    std::ofstream outFile;	//定义ofstream对象outFile
	outFile.open("marker.tcl");	//打开文件
	outFile << "gtkwave::setNamedMarker Z "<<error_time<<" DiffTestError!"<<endl;
	outFile << "gtkwave::setMarker "<<error_time<<endl;
	outFile.close();
}
void cemu_perf_diff(Vmycpu_top *top, axi4_ref &cpu_axi_ref) {/*{{{*/

    // cemu init{{{
    memory_bus cemu_mmio;
    
    Pmem cemu_func_mem(262144*4, "../nscscc-group/perf_test_v0.01/soft/perf_func/obj/allbench/inst_data.bin");
    cemu_func_mem.set_allow_warp(true);
    assert(cemu_mmio.add_dev(0x1fc00000,0x100000  ,&cemu_func_mem));
    assert(cemu_mmio.add_dev(0x00000000,0x10000000,&cemu_func_mem));
    assert(cemu_mmio.add_dev(0x20000000,0x20000000,&cemu_func_mem));
    assert(cemu_mmio.add_dev(0x40000000,0x40000000,&cemu_func_mem));
    assert(cemu_mmio.add_dev(0x80000000,0x80000000,&cemu_func_mem));

    nscscc_confreg cemu_confreg(false);
    assert(cemu_mmio.add_dev(0x1faf0000,0x10000,&cemu_confreg));

    mips_core cemu_mips(cemu_mmio);
    // cemu }}}

    // rtl soc-simulator init{{{
    axi4      mmio_sigs;
    axi4_ref  mmio_sigs_ref(mmio_sigs);
    axi4_xbar mmio(5);

    // perf mem at 0x1fc00000
    Pmem perf_mem(262144*4, "../nscscc-group/perf_test_v0.01/soft/perf_func/obj/allbench/inst_data.bin");
    assert(mmio.add_dev(0x1fc00000,0x100000,&perf_mem));

    // confreg at 0x1faf0000
    nscscc_confreg confreg(false);
    assert(mmio.add_dev(0x1faf0000,0x10000,&confreg));
    
    // connect Vcd for trace
    VerilatedVcdC vcd;
    IFDEF(CONFIG_TRACE_ON,top->trace(&vcd,0));
    uint64_t ticks = 0;
    // rtl soc-simulator }}}

    for (int test=CONFIG_PERF_START; test<=CONFIG_PERF_END; test++) {
        running = true;
        confreg.set_switch(test);
        cemu_confreg.set_switch(test);
        top->aresetn = 0;
        std::stringstream ss;
        ss << "trace-perf-" << test << ".vcd";
        ss.str();
        IFDEF(CONFIG_TRACE_ON,vcd.open(ss.str().c_str()));
        uint64_t rst_ticks = 5;
        uint64_t last_commit = ticks;
        uint64_t commit_timeout = 1024;
        top->aclk = 0;
        cemu_mips.reset();
        while (!Verilated::gotFinish()) {

            if (rst_ticks  > 0) {
                top->aresetn = 0;
                rst_ticks --;
                mmio.reset();
            }
            else top->aresetn = 1;
            top->aclk = !top->aclk;

            bool should_update = top->aclk && top->aresetn;
            if (should_update) {
                mmio_sigs.update_input(cpu_axi_ref);
            }
            top->eval();
            if (should_update) {
                confreg.tick();
                if (!mmio.beat(mmio_sigs_ref)) {
                    mmio.print_error();
                    break;
                }
                mmio_sigs.update_output(cpu_axi_ref);
                cemu_mips.update_cp0(0);
                cemu_confreg.tick();
            }
            IFDEF(CONFIG_TRACE_ON,vcd.dump(ticks));

            // trace with cemu {{{
            uint8_t commit_num = dpi_retire();
            if (commit_num > 0 && should_update){
                if (!running) break;
                CPU_state ref_r, mycpu;
                vtop_getState(&mycpu);
                for (uint8_t i = 0; i < commit_num; i++) {
                    cemu_mips.step();
                    if (cemu_mips.debug_wb_is_timer){
                        cemu_mips.set_GPR(cemu_mips.debug_wb_wnum, dpi_regfile(cemu_mips.debug_wb_wnum));
                    }
                    if (cemu_mips.debug_wb_pc == 0x1fc00380) {
                        printf("PASS TEST!\n");
                        running = false;
                    }
                }
                ref_r.pc = cemu_mips.debug_wb_pc;
                memcpy(ref_r.gpr, cemu_mips.GPR,4*32);
                if (!check_state(&mycpu, &ref_r)){
                    running = false;
                    print_diff(&mycpu, &ref_r);
                    write_tcl_marker(ticks);
                }
                last_commit = ticks;
            }
            if (ticks - last_commit >= commit_timeout) {
                printf("ERROR: There are %lu cycles since last commit\n", commit_timeout);
                running = false;
            }
            while (diff_uart && confreg.has_uart() && cemu_confreg.has_uart()) {
                uint8_t mycpu_uart = confreg.get_uart();
                uint8_t ref_uart = cemu_confreg.get_uart();
                if (mycpu_uart != ref_uart) {
                    printf("ERROR!\n UART different at %lu ticks.\n", ticks);
                    printf("Expected: %08x, found: %08x\n", mycpu_uart, ref_uart);
                    running = false;
                }
                else {
                    // printf("%c == %x\n",mycpu_uart,mycpu_uart);
                    putchar(mycpu_uart);
                    if ((mycpu_uart & 0x7f) ==0x7f){
                        printf("PASS TEST!\n");
                        running = false;
                    }
                }
            }
            // trace with cemu }}}
            ticks ++;
        }
        IFDEF(CONFIG_TRACE_ON,vcd.close());
    }
    top->final();
    printf("total ticks = %lu\n", ticks);
}/*}}}*/

int main(int argc, char** argv, char** env) {/*{{{*/
    Verilated::commandArgs(argc, argv);

    std::signal(SIGINT, [](int) {
        running = false;
    });

    Verilated::traceEverOn(CONFIG_TRACE_ON);
    // setup soc
    Vmycpu_top *top = new Vmycpu_top;
    pc = &(top->debug_wb_pc);
    axi4_ptr mmio_ptr;

    connect_wire(mmio_ptr,top);
    assert(mmio_ptr.check());
    int perf_start = 1;
    int perf_end = 10;
    
    axi4_ref mmio_ref(mmio_ptr);
    cemu_perf_diff(top, mmio_ref);
    return 0;
}/*}}}*/
