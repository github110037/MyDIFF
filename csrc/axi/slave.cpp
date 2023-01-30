#include "axi.hpp"
#include "common.hh"
axi4_slave::axi4_slave(int delay):delay(delay) {}
bool axi4_slave::beat(axi4_ref &pin) {/*{{{*/
    if (!read_channel(pin)) return false;
    if (!write_channel(pin)) return false;
    return true;
}/*}}}*/
void axi4_slave::reset() {/*{{{*/
    read_busy = false;
    read_last = false;
    read_wait = false;
    read_delay = 0;
    write_busy = false;
    b_busy     = false;
    write_delay = 0;
    r_ecode = NO_ERROR;
    w_ecode = NO_ERROR;
}/*}}}*/
void axi4_slave::print_error(){/*{{{*/
    switch (r_ecode) {
        case NO_BURST_TYPE:     
            printf("arburst is no legal but %x\n",r_burst_type);
            break;
        case WRAP_NOT_ALIGN:
            printf("arburst is wrap and address is not align\n");
            break;
        case BURST_UP_4K:
            printf("read burst total bytes is greater than 4KB\n");
            break;
        case SIZE_UP_WID:
            printf("read size is larger than data bus width\n");
            break;
        case NO_WARP_LEN:
            printf("arburst is wrap but arlen is not in {2,4,8,16}\n");
            break;
        case NO_ERROR:
            printf("read trascation is no error\n");
            break;
    }
    switch (w_ecode) {
        case NO_BURST_TYPE:     
            printf("awburst is no legal but %x\n",r_burst_type);
            break;
        case WRAP_NOT_ALIGN:
            printf("awburst is wrap and address is not align\n");
            break;
        case BURST_UP_4K:
            printf("write burst total bytes is greater than 4KB\n");
            break;
        case SIZE_UP_WID:
            printf("write size is larger than data bus width\n");
            break;
        case NO_WARP_LEN:
            printf("awburst is wrap but arlen is not in {2,4,8,16}\n");
            break;
        case NO_ERROR:
            printf("write trascation is no error\n");
            break;
    }
}/*}}}*/

bool axi4_slave::read_check() {/*{{{*/
    r_ecode = NO_ERROR;
    if (r_burst_type == BURST_RESERVED) r_ecode = NO_BURST_TYPE;
    if (r_burst_type == BURST_WRAP && (r_current_addr % r_each_len)) r_ecode = WRAP_NOT_ALIGN;
    if (r_burst_type == BURST_WRAP) {
        if (r_nr_trans != 2 && r_nr_trans != 4 && r_nr_trans != 8 && r_nr_trans != 16) 
            r_ecode = NO_WARP_LEN;
    }
    word_t rem_addr = 4096 - (r_start_addr % 4096);
    if (r_tot_len > rem_addr) r_ecode = BURST_UP_4K;
    if (r_each_len > D_bytes) r_ecode = SIZE_UP_WID;
    return r_ecode==NO_ERROR;
}/*}}}*/
void axi4_slave::read_beat(axi4_ref &pin) {/*{{{*/
    pin.rid = arid;
    pin.rvalid  = 1;
    bool update = false;
    if (pin.rready || r_cur_trans == 0) {
        r_cur_trans += 1;
        update = true;
        if (r_cur_trans == r_nr_trans) {
            read_last = true;
            read_busy = false;
        }
    }
    pin.rlast = read_last;
    if (update) {
        if (r_early_err) {
            pin.rresp = RESP_DECERR;
            pin.rdata = 0;
        }
        else if (r_burst_type == BURST_FIXED) {
            pin.rresp = paddrTop.do_read(r_start_addr, r_tot_len, &r_data[r_start_addr % 4096]);
            pin.rdata = *(AUTO_T(CONFIG_AXI_DWID)*)(&r_data[(r_start_addr % 4096) - (r_start_addr % D_bytes)]);
        }
        else { // INCR, WRAP
            pin.rresp = r_resp;
            pin.rdata = *(AUTO_T(CONFIG_AXI_DWID)*)(&r_data[r_current_addr - (r_current_addr % D_bytes)]);
            r_current_addr += r_each_len - (r_current_addr % r_each_len);
            if (r_burst_type == BURST_WRAP && r_current_addr == ((r_start_addr % 4096) + r_each_len * r_nr_trans)) {
                r_current_addr = r_start_addr % 4096;
            }
        }
    }
}/*}}}*/
bool axi4_slave::read_init(axi4_ref &pin) {/*{{{*/
    arid            = static_cast<unsigned int>(pin.arid);
    r_burst_type    = static_cast<axi_burst_type>(pin.arburst);
    r_each_len      = 1 << pin.arsize;
    r_nr_trans      = pin.arlen + 1;
    r_current_addr  = (r_burst_type == BURST_WRAP) ? (pin.araddr % 4096) : ((pin.araddr % 4096) - (pin.araddr % r_each_len));
    r_start_addr    = (r_burst_type == BURST_WRAP) ? (pin.araddr - (pin.araddr % (r_each_len * r_nr_trans) ) ) : pin.araddr;
    r_cur_trans     = 0;
    r_tot_len       = ( (r_burst_type == BURST_FIXED) ? r_each_len : r_each_len * r_nr_trans) - (r_start_addr % r_each_len); // first beat can be unaligned
    r_early_err     = !read_check();
    if (!r_early_err){
        // clear unused bits.
        if (r_start_addr % D_bytes) {
            word_t clear_addr = r_start_addr % 4096;
            clear_addr -= clear_addr % D_bytes;
            memset(&r_data[clear_addr],0x00,D_bytes);
        }
        if ((r_start_addr + r_tot_len) % D_bytes) {
            word_t clear_addr = (r_start_addr + r_tot_len) % 4096;
            clear_addr -= (clear_addr % D_bytes);
            memset(&r_data[clear_addr],0x00,D_bytes);
        }
        // For BURST_FIXED, we call do_read every read burst
        if (r_burst_type != BURST_FIXED) 
            r_resp = do_read(static_cast<word_t>(r_start_addr), static_cast<word_t>(r_tot_len), &r_data[r_start_addr % 4096] );
    }
    return !r_early_err;
}/*}}}*/
bool axi4_slave::read_channel(axi4_ref &pin) {/*{{{*/
    bool res = true;
    do {
        // Read step 1. release old transaction
        if (read_last && pin.rready) {
            read_last = false;
            pin.rvalid = 0;     // maybe change in the following code
            pin.rlast = 0;
            if (read_wait) {
                read_wait = false;
                read_busy = true;
                read_delay = delay;
            }
        }
        // Read step 2. check new address come
        if (pin.arready && pin.arvalid) {

            res = read_init(pin);
            if (!res) break;

            if (read_last) read_wait = true;//HACK:AR_channal handshake ok require read_wait = false
            else {
                read_busy = true;
                read_delay = delay;
            }
        }
        // Read step 3. do read trascation
        if (read_busy) {
            if (read_delay) read_delay --;
            else read_beat(pin);
        }
        // Read step 4. set arready before new address come, it will change read_busy and read_wait status
        pin.arready = !read_busy && !read_wait;
    } while(0);
    return res;
}/*}}}*/

bool axi4_slave::write_check() {/*{{{*/
    w_ecode = NO_ERROR;
    if (w_burst_type == BURST_RESERVED) w_ecode = NO_BURST_TYPE;
    if (w_burst_type == BURST_WRAP && (w_current_addr % w_each_len)) r_ecode = WRAP_NOT_ALIGN;
    if (w_burst_type == BURST_WRAP) {
        if (w_nr_trans != 2 || w_nr_trans != 4 || w_nr_trans != 8 || w_nr_trans != 16)
            r_ecode = NO_WARP_LEN;
    }
    word_t rem_addr = 4096 - (w_start_addr % 4096);
    if (w_tot_len > rem_addr) r_ecode = BURST_UP_4K;
    if (w_each_len > D_bytes) r_ecode = SIZE_UP_WID;
    return w_ecode==NO_ERROR;
}/*}}}*/
bool axi4_slave::write_init(axi4_ref &pin) {/*{{{*/
    awid            = pin.awid;
    w_burst_type    = static_cast<axi_burst_type>(pin.awburst);
    w_each_len      = 1 << pin.awsize;
    w_nr_trans      = pin.awlen + 1;
    w_current_addr  = (w_burst_type == BURST_WRAP) ? pin.awaddr : (pin.awaddr - (pin.awaddr % w_each_len));
    w_start_addr    = (w_burst_type == BURST_WRAP) ? (pin.awaddr - (pin.awaddr % (w_each_len * w_nr_trans))) : pin.awaddr;
    w_cur_trans     = 0;
    w_tot_len       = w_each_len * w_nr_trans - (w_start_addr % w_each_len);
    w_early_err     = !write_check();
    w_resp          = RESP_OKEY;
    return !w_early_err;
}/*}}}*/
std::vector< std::pair<int,int> > axi4_slave::strb_to_range (AUTO_T(CONFIG_AXI_DWID/8) wstrb, int st_pos, int ed_pos){/*{{{*/
    std::vector<std::pair<int,int> > res;
    int l = st_pos;
    while (l < ed_pos) {
        if ((wstrb >> l) & 1) {
            int r = l;
            while ((wstrb >> r) & 1 && r < ed_pos) r ++;
            res.emplace_back(l,r-l);
            l = r + 1;
        }
        else l ++;
    }
    return res;
}/*}}}*/
bool axi4_slave::write_beat(axi4_ref &pin) {/*{{{*/
    if (pin.wvalid && pin.wready) {
        w_cur_trans += 1;
        if (w_cur_trans == w_nr_trans) {
            write_busy = false;
            b_busy = true;
            w_early_err |= !pin.wlast;
        }
        if (!w_early_err){
            word_t addr_base = w_current_addr;
            if (w_burst_type != BURST_FIXED) {
                w_current_addr += w_each_len - (addr_base % w_each_len);
                if (w_current_addr == (w_start_addr + w_each_len * w_nr_trans)) w_cur_trans =  w_start_addr; // warp support
            }
            word_t in_data_pos = addr_base % D_bytes;
            addr_base -= addr_base % D_bytes;
            word_t rem_data_pos = w_each_len - (in_data_pos % w_each_len); // unaligned support
                                                                             // split discontinuous wstrb bits to small requests
            std::vector<std::pair<int,int> > range = strb_to_range(pin.wstrb,in_data_pos,in_data_pos+rem_data_pos);
            for (std::pair<int,int> sub_range : range) {
                int &addr = sub_range.first;
                int &len  = sub_range.second;
                memcpy(w_buffer,&(pin.wdata),sizeof(pin.wdata));
                w_resp = static_cast<axi_resp>(static_cast<int>(w_resp) | static_cast<int>(do_write(addr_base+addr,len,w_buffer+addr)));
            }
        }
    }
    return !w_early_err;
}/*}}}*/
void axi4_slave::b_beat(axi4_ref &pin) {/*{{{*/
    pin.bid = awid;
    pin.bresp = w_early_err ? RESP_DECERR : w_resp;
    if (pin.bvalid && pin.bready) b_busy = false;
}/*}}}*/
bool axi4_slave::write_channel(axi4_ref &pin) {/*{{{*/
    bool res = true;
    do {
        if (pin.awready && pin.awvalid) {
            res = write_init(pin);
            write_busy = true;
            write_delay = delay;
            if (!res) break;
        }
        if (write_busy) {
            if (write_delay) write_delay --;
            else res = write_beat(pin);
            if (!res) break;
        }
        if (b_busy) {
            b_beat(pin);
        }
        pin.bvalid = b_busy;
        pin.awready = !write_busy && !b_busy;
        if (delay) pin.wready = write_busy && !write_delay;
        else pin.wready = !b_busy;
    } while(0) ;
    return res;
}/*}}}*/
