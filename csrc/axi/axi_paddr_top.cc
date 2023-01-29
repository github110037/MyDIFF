#include "axi.hpp"
#include "PaddrInterface.hpp"
#include "../csrc/PaddrTop.cc"
class axi_paddr_top{/*{{{*/
    public:
        axi_paddr_top(int delay = 0);
        bool beat(axi4_ref &pin);
        void reset();
        void print_error();
        PaddrTop paddrTop;

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
        uint64_t        r_start_addr;   // lower bound of transaction address
        uint64_t        r_current_addr; // current burst address in r_data buffer (physical address % 4096)
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
        uint64_t        w_start_addr;
        uint64_t        w_current_addr;
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
