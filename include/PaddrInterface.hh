#ifndef __MMIO_DEV__
#define __MMIO_DEV__
#include <stdint.h>
#include <cassert>
#include <vector>
#include <queue>
#include "common.hh"
#include "utils.hh"
#include <stdio.h>
// address interval
class AddrIntv {
    public:
    // example: [0x100,0x1ff]
    word_t start;   // include:0x01000
    word_t mask;
    AddrIntv(word_t _start, word_t _len): start(_start), mask(_len - 1){
        Assert((start & mask) == 0,"AddrIntv parameter error: start:%x\tlen:\t%x\n",_start,_len);
    };
    AddrIntv(word_t _start, uint8_t mask_bits):start (_start), mask(BITMASK(mask_bits)){
        Assert((start & mask) == 0,"AddrIntv parameter error: start:%x\tmask_len:\t%x\n",_start,mask_bits);
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
class PaddrTop: public PaddrInterface{/*{{{*/
    private:
        std::vector<std::pair<AddrIntv, PaddrInterface*>> devices;
    public:
        PaddrTop();
        bool add_dev(AddrIntv &new_range, PaddrInterface *dev);
        bool do_read (word_t addr, size_wstrb info, word_t* data);
        bool do_write(word_t addr, size_wstrb info, const word_t data);
};/*}}}*/
class Pmem : public PaddrInterface  {/*{{{*/
    private:
        unsigned char *mem;
        size_t mem_size;
    public:
        Pmem(word_t size_bytes);
        Pmem(const AddrIntv &_range);
        Pmem(size_t size_bytes, const unsigned char *init_binary, size_t init_binary_len);
        Pmem(size_t size_bytes, const char *init_file);
        ~Pmem() ;
        bool do_read (word_t addr, size_wstrb info, word_t* data);
        bool do_write(word_t addr, size_wstrb info, const word_t data);
        void load_binary(uint64_t addr, const char *init_file);
        void save_binary(const char *filename) ;
        uint8_t *get_mem_ptr();
};/*}}}*/
// offset += 0x1faf8000 {{{
#define CR0_ADDR            0x8000  //32'hbfaf_8000 
#define CR1_ADDR            0x8004  //32'hbfaf_8004 
#define CR2_ADDR            0x8008  //32'hbfaf_8008 
#define CR3_ADDR            0x800c  //32'hbfaf_800c 
#define CR4_ADDR            0x8010  //32'hbfaf_8010 
#define CR5_ADDR            0x8014  //32'hbfaf_8014 
#define CR6_ADDR            0x8018  //32'hbfaf_8018 
#define CR7_ADDR            0x801c  //32'hbfaf_801c 
#define LED_ADDR            0xf000  //32'hbfaf_f000 
#define LED_RG0_ADDR        0xf004  //32'hbfaf_f004 
#define LED_RG1_ADDR        0xf008  //32'hbfaf_f008 
#define NUM_ADDR            0xf010  //32'hbfaf_f010 
#define SWITCH_ADDR         0xf020  //32'hbfaf_f020 
#define BTN_KEY_ADDR        0xf024  //32'hbfaf_f024
#define BTN_STEP_ADDR       0xf028  //32'hbfaf_f028
#define SW_INTER_ADDR       0xf02c  //32'hbfaf_f02c 
#define TIMER_ADDR          0xe000  //32'hbfaf_e000 
#define IO_SIMU_ADDR        0xffec  //32'hbfaf_ffec
#define VIRTUAL_UART_ADDR   0xfff0  //32'hbfaf_fff0
#define SIMU_FLAG_ADDR      0xfff4  //32'hbfaf_fff4 
#define OPEN_TRACE_ADDR     0xfff8  //32'hbfaf_fff8
#define NUM_MONITOR_ADDR    0xfffc  //32'hbfaf_fffc

// physical address = [0x1faf0000,0x1fafffff]
class PaddrConfreg: public PaddrInterface {
    private:
        uint32_t cr[8];
        uint32_t switch_data;
        uint32_t switch_inter_data;
        uint32_t timer;
        uint32_t led;
        uint32_t led_rg0;
        uint32_t led_rg1;
        uint32_t num;
        uint32_t simu_flag;
        uint32_t io_simu;
        uint8_t virtual_uart;
        uint32_t open_trace;
        uint32_t num_monitor;
        std::queue <uint8_t> uart_queue;
    public:
        uint32_t confreg_read = 0;
        uint32_t confreg_write = 0;
        PaddrConfreg(bool simulation = false);
        void tick();
        bool do_read (word_t addr, size_wstrb info, word_t* data);
        bool do_write(word_t addr, size_wstrb info, const word_t data);
        void set_switch(uint8_t value);
        bool has_uart();
        char get_uart();
        uint32_t get_num();
};/*}}}*/
#endif
