#include "PaddrInterface.hh"

PaddrTop::PaddrTop(){
    devices.clear();
};

bool PaddrTop::add_dev(AddrIntv &new_range, PaddrInterface *dev) {
    // check overlap
    for (auto it: devices){
        AddrIntv old_range = it.first;
        word_t l_max = std::max(old_range.start,new_range.start);
        word_t r_min = std::min(old_range.end(),new_range.end());
        if (l_max < r_min) return false; // overleap
    }
    devices.push_back(std::make_pair(new_range, dev));
    return true;
}

bool PaddrTop::do_read (word_t addr, size_wstrb info, word_t* data){
    for (auto it: devices){
        AddrIntv dev_range = it.first;
        if (dev_range.start<=addr && addr+info.size<=dev_range.end()){
            return it.second->do_read(addr & dev_range.mask, info, data);
        }
    }
    return false;
}
bool PaddrTop::do_write(word_t addr, size_wstrb info, const word_t data){
    for (auto it: devices){
        AddrIntv dev_range = it.first;
        if (dev_range.start<=addr && addr+info.size<=dev_range.end()){
            return it.second->do_write(addr & dev_range.mask, info, data);
        }
    }
    return false;
}
