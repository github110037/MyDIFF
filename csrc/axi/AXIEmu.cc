#include "Vmycpu_top.h"
#include "verilated.h"
#include "macro.hpp"
#include "generated/autoconf.h"
#include <cstdlib>
#include <memory>
#include "PaddrTop.hh"
typedef enum{
    SIM_RUN,
    SIM_ABORT,
}sim_status_t;
sim_status_t sim_status;
#define __ASSERT_SIM__(cond,fmt,...) if (!(cond)) {\
    Log(fmt, ## __VA_ARGS__);\
    sim_status = SIM_ABORT;}

typedef enum {/*{{{*/
    BURST_FIXED = 0,
    BURST_INCR  = 1,
    BURST_WRAP  = 2,
    BURST_RESERVED  = 3
}burst_t;/*}}}*/
typedef enum {/*{{{*/
    RESP_OKEY   = 0,
    RESP_EXOKEY = 1,
    RESP_SLVERR = 2,
    RESP_DECERR = 3
}resp_t ;/*}}}*/
// AXI_BUNDLE{{{
#define AUTO_T(width) \
    typename std::conditional <(width) <=  8, CData, \
    typename std::conditional <(width) <= 16, SData, \
    typename std::conditional <(width) <= 32, IData, QData >::type >::type >::type
#define  __M_IN__ 1
#define  __M_OUT__ 0
#define AXI_BUNDLE(_,...) \
    _(CONFIG_AXI_IDWID  ,arid   , __M_OUT__) __VA_ARGS__\
    _(CONFIG_AXI_AWID   ,araddr , __M_OUT__) __VA_ARGS__\
    _(8                 ,arlen  , __M_OUT__) __VA_ARGS__\
    _(3                 ,arsize , __M_OUT__) __VA_ARGS__\
    _(2                 ,arburst, __M_OUT__) __VA_ARGS__\
    _(1                 ,arvalid, __M_OUT__) __VA_ARGS__\
    _(1                 ,arready, __M_IN__ ) __VA_ARGS__\
    \
    _(CONFIG_AXI_IDWID  ,rid    , __M_IN__ ) __VA_ARGS__\
    _(CONFIG_AXI_DWID   ,rdata  , __M_IN__ ) __VA_ARGS__\
    _(2                 ,rresp  , __M_IN__ ) __VA_ARGS__\
    _(1                 ,rlast  , __M_IN__ ) __VA_ARGS__\
    _(1                 ,rvalid , __M_IN__ ) __VA_ARGS__\
    _(1                 ,rready , __M_OUT__) __VA_ARGS__\
    \
    _(CONFIG_AXI_IDWID  ,awid   , __M_OUT__) __VA_ARGS__\
    _(CONFIG_AXI_AWID   ,awaddr , __M_OUT__) __VA_ARGS__\
    _(8                 ,awlen  , __M_OUT__) __VA_ARGS__\
    _(3                 ,awsize , __M_OUT__) __VA_ARGS__\
    _(2                 ,awburst, __M_OUT__) __VA_ARGS__\
    _(1                 ,awvalid, __M_OUT__) __VA_ARGS__\
    _(1                 ,awready, __M_IN__ ) __VA_ARGS__\
    \
    _(CONFIG_AXI_IDWID  ,wid    , __M_OUT__) __VA_ARGS__\
    _(CONFIG_AXI_DWID   ,wdata  , __M_OUT__) __VA_ARGS__\
    _(CONFIG_AXI_DWID/8 ,wstrb  , __M_OUT__) __VA_ARGS__\
    _(1                 ,wlast  , __M_OUT__) __VA_ARGS__\
    _(1                 ,wvalid , __M_OUT__) __VA_ARGS__\
    _(1                 ,wready , __M_IN__ ) __VA_ARGS__\
    \
    _(CONFIG_AXI_IDWID  ,bid    , __M_IN__ ) __VA_ARGS__\
    _(2                 ,bresp  , __M_IN__ ) __VA_ARGS__\
    _(1                 ,bvalid , __M_IN__ ) __VA_ARGS__\
    _(1                 ,bready , __M_OUT__)
/*}}}*/
#define __my_axi_ref_def__(width,name,masterIn) IFZERO(masterIn,const) AUTO_T(width)& name;
#define __my_axi_ref_ini__(width,name,masterIn) name (mycpu->name)
#define __my_axi_out_def__(width,name,masterIn) IFONE(masterIn, AUTO_T(width) s_##name;)
#define __my_axi_out_ref__(width,name,masterIn) IFONE(masterIn, pins.name = s_##name;)
#define __comm__ ,
class MyAXIRef{
    public:
        AXI_BUNDLE(__my_axi_ref_def__)
        MyAXIRef(Vmycpu_top *mycpu):
            AXI_BUNDLE(__my_axi_ref_ini__, __comm__) {}
};
class MyAXIPaddr{
    private:
        MyAXIRef pins;
        PaddrTop paddr;
        AXI_BUNDLE(__my_axi_out_def__)
            uint8_t delay;
    public:
        MyAXIPaddr(Vmycpu_top *mycpu):
            pins(MyAXIRef(mycpu)) {}
        bool calculate_output(){
            bool res = read_eval();
            res &= write_eval();
            return res;
        }
        void update_output(){
            AXI_BUNDLE(__my_axi_out_ref__)
        }
        void reset(){
            r_status = r_idel;
        }

    private:
        bool check_axi_req(uint8_t num_bytes, burst_t burst_type, word_t start_addr, uint8_t burst_len){/*{{{*/
            bool res = true;
            __ASSERT_SIM__(num_bytes<=(CONFIG_AXI_DWID>>8), \
                    "32 AXI read bytes number not support %d",num_bytes);
            __ASSERT_SIM__(burst_type!=BURST_RESERVED, \
                    "Arburst type is RESERVED");

            word_t align_addr = start_addr & ~(num_bytes-1);
            bool aligned = start_addr==align_addr;

            if (burst_type==BURST_WRAP){
                __ASSERT_SIM__(aligned, "Arburst type is WRAP but not aligned");
                __ASSERT_SIM__((burst_len==2 || burst_len==4 || burst_len==8 || burst_len==16), \
                        "Arburst type is WRAP but arlen is %d", burst_len);
                // NOTE:wrap type must not cross 4KB bound for wrapping at 4KB
            }
            else {
                __ASSERT_SIM__(align_addr>>12==(align_addr + num_bytes * burst_len - 1)>>12,\
                        "Arburst type is %d but cross 4KB address bound",burst_type);
                if (burst_len) {__ASSERT_SIM__(aligned, "This enviroment do not support unalign burst");}
            }//NOTE: FIX must not cross 4KB address bound
            return res;
        }/*}}}*/
        typedef enum{/*{{{*/
            r_idel,     // no transition; arready = 1
            r_req_ok,   // see arready and arvalid are set in posedge, and delay not end; arready = rvalid = 0;
            r_wait_last,// delay end and set rvalid but not (rready and rlast); rvalid = 1;
                        //
        } rstatus_t;/*}}}*/
        typedef enum{/*{{{*/
            w_idel,     // no transition; arready = 1, but wready = 0;
            w_req_ok,   // wait to accept wdata and info; wready always 1, but if id error sim will stop
            w_data_ok,  // see wready, wvalid, wlast set in posedge, start delay; bvalid and awready = 0
            w_wait_back,// delay is end, wait bready to reset; bvalid = 1;
        } wstatus_t;/*}}}*/

        rstatus_t r_status;
        uint8_t r_burst_count;
        uint8_t r_left_time;
        word_t r_wrap_off_mask;
        word_t r_wrap_bound;
        burst_t r_burst_type;

        word_t r_cur_addr;
        size_wstrb r_cur_info;
        uint8_t r_cur_id;

        bool accept_read_req(){/*{{{*/
            bool res = true;
            uint8_t num_bytes = 1 << pins.arsize;

            r_burst_count = pins.arlen + 1;
            word_t start_addr = pins.araddr;
            r_burst_type = (burst_t)pins.arburst;

            check_axi_req(num_bytes, r_burst_type, start_addr, r_burst_count);

            if (r_burst_type==BURST_WRAP){
                r_wrap_off_mask = ((r_burst_count)<<pins.arsize)-1;
                r_wrap_bound = start_addr & r_wrap_off_mask;
            }
            r_cur_id = pins.arid;
            r_cur_addr = start_addr;
            r_cur_info.size = num_bytes;

            r_left_time = rand_delay();
            s_arready = 0;
            return res;
        } // check axi, assign last_count, assign left_time, unset arready }}}
        bool do_once_read(){/*{{{*/
            bool res = true;
            res = paddr.do_read(r_cur_addr, r_cur_info, &s_rdata);
            s_rvalid = 1;
            s_rlast = r_burst_count==0;
            s_rid = r_cur_id;
            s_rresp = res ? RESP_OKEY : RESP_DECERR;
            r_burst_count--;

            switch (w_burst_type) {
                case BURST_FIXED:
                    break;
                case BURST_INCR:
                    r_cur_addr += r_cur_info.size;
                case BURST_WRAP:
                    r_cur_addr = ((r_cur_addr + r_cur_info.size) & r_wrap_off_mask) | r_wrap_bound;
                default:
                    assert(0);
            }
            return res;
        } // check axi, last_count--, assign axi R channel valid}}}
        void idel_wait_read(){/*{{{*/
            s_rvalid = 0;
            s_rresp = 0;
            s_rid = 0;
            s_rlast = 0;
            s_rdata = 0;
            s_arready = 1;
        };/*}}}*/
        bool read_eval(){/*{{{*/
            bool res = true;
            switch (r_status) {
                case r_idel: 
                    if (pins.arvalid){
                        r_status = r_req_ok;
                        res &= accept_read_req();
                    }
                    break;
                case r_req_ok:
                    if (r_left_time) r_left_time--;
                    else {
                        r_status = r_wait_last;
                        res &= do_once_read();
                    }
                case r_wait_last:
                    if (pins.rready && pins.rlast) {
                        r_status = r_idel;
                        idel_wait_read();
                    }
                    else res &= do_once_read();
                default:
                    assert(0);
            }
            return res;
        }/*}}}*/
        
        wstatus_t w_status;
        uint8_t w_burst_count;
        uint8_t w_left_time;
        word_t w_wrap_off_mask;
        word_t w_wrap_bound;
        burst_t w_burst_type;

        word_t w_cur_addr[16];
        size_wstrb w_cur_info[16];
        uint8_t w_cur_id;
        word_t w_cur_data[16];
        uint8_t w_cur_NO;

        bool accept_write_req(){/*{{{*/
            bool res = true;
            uint8_t num_bytes = 1 << pins.awsize;

            w_burst_count = pins.awlen + 1;
            word_t start_addr = pins.awaddr;
            w_burst_type = (burst_t)pins.awburst;

            check_axi_req(num_bytes, w_burst_type, start_addr, w_burst_count);

            if (w_burst_type==BURST_WRAP){
                w_wrap_off_mask = ((w_burst_count)<<pins.awsize)-1;
                w_wrap_bound = start_addr & w_wrap_off_mask;
            }

            w_cur_NO = 0;
            w_cur_id = pins.awid;
            w_cur_addr[w_cur_NO] = start_addr;
            w_cur_info[w_cur_NO].size = num_bytes;
            s_awready = 0;
            s_wready = 1;
            return res;
        };/*}}}*/
        bool accept_write_data(){/*{{{*/
            bool res = true;
            __ASSERT_SIM__(pins.wid==w_cur_id, "Write data %x wid != awid",pins.wdata);
            w_cur_data[w_cur_NO] = pins.wdata;
            w_cur_info[w_cur_NO].wstrb = pins.wstrb;
            w_cur_NO++;
            if (w_cur_NO == w_burst_count-1) {
                __ASSERT_SIM__(pins.wlast==1, "Write data %x wlast != 1 when the last wdata arrive",pins.wdata);
                s_wready = 0;
                w_left_time = rand_delay();
            }
            else {
                __ASSERT_SIM__(pins.wlast==0, "Write data %x wlast is set but not the last transition",pins.wdata);
                word_t last_addr = w_cur_addr[w_cur_NO-1];
                uint8_t last_size= w_cur_info[w_cur_NO-1].size;
                w_cur_info[w_cur_NO].size = last_size;
                switch (w_burst_type) {
                    case BURST_FIXED:
                        w_cur_addr[w_cur_NO] = last_addr;
                        break;
                    case BURST_INCR:
                        w_cur_addr[w_cur_NO] = last_addr + last_size;
                        break;
                    case BURST_WRAP:
                        w_cur_addr[w_cur_NO] = ((last_addr+last_size) & w_wrap_off_mask) | w_wrap_bound;
                        break;
                    default:
                        assert(0);
                }
            }
            return res;
        };/*}}}*/
        bool do_all_write(){/*{{{*/
            bool res = true;
            for (size_t i = 0; i < w_burst_count; i++) {
                res &= paddr.do_write(w_cur_addr[i], w_cur_info[i], w_cur_data[i]);
            }
            s_bvalid = 1;
            s_bresp = res ? RESP_OKEY : RESP_DECERR;
            s_bid = w_cur_id;
            return res;
        };/*}}}*/
        void idel_wait_write(){/*{{{*/
            s_awready = 1;
            s_wready = 0;
            s_bid = 0;
            s_bresp = 0;
            s_bvalid = 0;
        };/*}}}*/
        bool write_eval(){/*{{{*/
            bool res = true;
            switch (w_status) {
                case w_idel:
                    if (pins.awvalid){
                        w_status = w_req_ok;
                        res &= accept_read_req();
                    }
                    break;
                case w_req_ok:
                    if (pins.wvalid){
                        res &= accept_write_data();
                        w_status = pins.wlast ? w_data_ok : w_req_ok;
                    }
                    break;
                case w_data_ok:
                    if (w_left_time) w_left_time--;
                    else {
                        w_status = w_wait_back;
                        res &= do_all_write();
                    }
                    break;
                case w_wait_back:
                    if (pins.bready){
                        w_status = w_idel;
                        idel_wait_write();
                    }
                    break;
            }
            return res;
        }/*}}}*/
        inline int rand_delay() {return rand()%16+16;}
};

using namespace std;
int main (int argc, char *argv[]) {
    unique_ptr<Vmycpu_top> top(new Vmycpu_top());
    unique_ptr<MyAXIPaddr> axi(new MyAXIPaddr(top.get()));
    return 0;
}
