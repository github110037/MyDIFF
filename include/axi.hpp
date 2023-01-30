#ifndef __AXI_HPP__
#define __AXI_HPP__
#include "generated/autoconf.h"
#include <verilated.h>
#include "macro.hpp"
#include "PaddrTop.hh"
#include <map>

// AXI_BUNDLE{{{
#define AUTO_T(width) \
    typename std::conditional <(width) <=  8, CData, \
    typename std::conditional <(width) <= 16, SData, \
    typename std::conditional <(width) <= 32, IData, QData >::type >::type >::type
#define AXI_BUNDLE(_,...) \
    _(CONFIG_AXI_IDWID  ,arid   , 0) __VA_ARGS__\
    _(CONFIG_AXI_AWID   ,araddr , 0) __VA_ARGS__\
    _(8                 ,arlen  , 0) __VA_ARGS__\
    _(3                 ,arsize , 0) __VA_ARGS__\
    _(2                 ,arburst, 0) __VA_ARGS__\
    _(1                 ,arvalid, 0) __VA_ARGS__\
    _(1                 ,arready, 1) __VA_ARGS__\
    _(CONFIG_AXI_IDWID  ,rid    , 1) __VA_ARGS__\
    _(CONFIG_AXI_DWID   ,rdata  , 1) __VA_ARGS__\
    _(2                 ,rresp  , 1) __VA_ARGS__\
    _(1                 ,rlast  , 1) __VA_ARGS__\
    _(1                 ,rvalid , 1) __VA_ARGS__\
    _(1                 ,rready , 0) __VA_ARGS__\
    _(CONFIG_AXI_IDWID  ,awid   , 0) __VA_ARGS__\
    _(CONFIG_AXI_AWID   ,awaddr , 0) __VA_ARGS__\
    _(8                 ,awlen  , 0) __VA_ARGS__\
    _(3                 ,awsize , 0) __VA_ARGS__\
    _(2                 ,awburst, 0) __VA_ARGS__\
    _(1                 ,awvalid, 0) __VA_ARGS__\
    _(1                 ,awready, 1) __VA_ARGS__\
    _(CONFIG_AXI_DWID   ,wdata  , 0) __VA_ARGS__\
    _(CONFIG_AXI_DWID/8 ,wstrb  , 0) __VA_ARGS__\
    _(1                 ,wlast  , 0) __VA_ARGS__\
    _(1                 ,wvalid , 0) __VA_ARGS__\
    _(1                 ,wready , 1) __VA_ARGS__\
    _(CONFIG_AXI_IDWID  ,bid    , 1) __VA_ARGS__\
    _(2                 ,bresp  , 1) __VA_ARGS__\
    _(1                 ,bvalid , 1) __VA_ARGS__\
    _(1                 ,bready , 0)
/*}}}*/
/*
   We have defined 3 types of AXI signals for a different purposes: axi4, axi4_ptr, axi4_ref.
   Since verilator exposes signals as a value itself, we use axi4_ptr to get signal to connect.
   Then axi4_ptr can be converted to axi4_ref to reach the value in a better way.
   */
class axi4_ref;
class axi4;
class axi4_ptr;

class axi4_ptr{/*{{{*/
#define __AXI4_PTR_DEF__(width,name,input) AUTO_T(width)  *name = NULL;
    public:
        AXI_BUNDLE(__AXI4_PTR_DEF__)
            // check all signals is not null and different
            bool check();
};/*}}}*/

class axi4{/*{{{*/
#define __AXI4_DEF__(width,name,input) AUTO_T(width) name = 0;
    public:
        AXI_BUNDLE(__AXI4_DEF__)

            // update slave input as master output
            void update_input (axi4_ref &ref);

        // update master input as slave output
        void update_output(axi4_ref &ref);
};/*}}}*/

class axi4_ref{/*{{{*/
#define __AXI4_REF_DEF__(width,name,input) AUTO_T(width) &name;
    public:
        AXI_BUNDLE(__AXI4_REF_DEF__)
            uint8_t last;
        axi4_ref(axi4_ptr &ptr);
        axi4_ref(axi4 &var);
};/*}}}*/

enum axi_resp {/*{{{*/
    RESP_OKEY   = 0,
    RESP_EXOKEY = 1,
    RESP_SLVERR = 2,
    RESP_DECERR = 3
};/*}}}*/

enum axi_burst_type {/*{{{*/
    BURST_FIXED = 0,
    BURST_INCR  = 1,
    BURST_WRAP  = 2,
    BURST_RESERVED  = 3
};/*}}}*/

class axi4_slave{/*{{{*/
    public:
        axi4_slave(int delay = 0);
        bool beat(axi4_ref &pin);
        void reset();
        void print_error();
        PaddrTop paddrTop();
    private:
        unsigned int D_bytes = CONFIG_AXI_DWID / 8;
        int delay;

        enum ecode_t {/*{{{*/
            NO_ERROR = 0,
            NO_BURST_TYPE,
            WRAP_NOT_ALIGN,
            NO_WARP_LEN,
            BURST_UP_4K,
            SIZE_UP_WID,
        };/*}}}*/

    private:
        bool read_busy = false; // during trascation except last
        bool read_last = false; // wait rready and free
        bool read_wait = false; // ar ready, but waiting the last read to ready
        int  read_delay = 0; // delay
        word_t        r_start_addr;   // lower bound of transaction address
        word_t        r_current_addr; // current burst address in r_data buffer (physical address % 4096)
        AUTO_T(CONFIG_AXI_IDWID) arid;
        axi_burst_type  r_burst_type;
        unsigned int    r_each_len;
        unsigned int    r_nr_trans;
        unsigned int    r_cur_trans; // current burst times 
        unsigned int    r_tot_len;  
        bool            r_out_ready;
        bool            r_early_err;
        ecode_t         r_ecode;
        axi_resp        r_resp;
        uint8_t         r_data[4096];
        bool read_check();
        void read_beat(axi4_ref &pin);
        bool read_init(axi4_ref &pin);
        bool read_channel(axi4_ref &pin);

    private:
        bool write_busy = false;
        bool b_busy     = false;
        int  write_delay = 0;
        word_t        w_start_addr;
        word_t        w_current_addr;
        AUTO_T(CONFIG_AXI_IDWID) awid;
        axi_burst_type  w_burst_type;
        unsigned int    w_each_len;
        int             w_nr_trans;
        int             w_cur_trans;
        unsigned int    w_tot_len;
        bool            w_out_ready;
        ecode_t         w_ecode;
        bool            w_early_err;
        axi_resp        w_resp;
        uint8_t         w_buffer[CONFIG_AXI_DWID/8];
        bool write_check();
        bool write_init(axi4_ref &pin);
        std::vector< std::pair<int,int> > strb_to_range (AUTO_T(CONFIG_AXI_DWID/8) wstrb, int st_pos, int ed_pos);
        bool write_beat(axi4_ref &pin);
        void b_beat(axi4_ref &pin);
        bool write_channel(axi4_ref &pin);
};/*}}}*/
#endif // !__AXI_HPP__
