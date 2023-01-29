#include "axi.hpp"
#include <fstream>
#include <iostream>
axi4_mem::axi4_mem(size_t size_bytes) {/*{{{*/
    if (size_bytes % (CONFIG_AXI_DWID/8)) size_bytes += 8 - (size_bytes % (CONFIG_AXI_DWID/8));
    mem = new unsigned char[size_bytes];
    mem_size = size_bytes;
}/*}}}*/
axi4_mem::axi4_mem(size_t size_bytes, const uint8_t *init_binary, size_t init_binary_len):axi4_mem(size_bytes) {/*{{{*/
    assert(init_binary_len <= size_bytes);
    memcpy(mem,init_binary,init_binary_len);
}/*}}}*/
axi4_mem::~axi4_mem() {/*{{{*/
    delete [] mem;
}/*}}}*/
bool axi4_mem::read(off_t start_addr, size_t size, uint8_t* buffer) {/*{{{*/
    if (start_addr + size <= mem_size) {
        memcpy(buffer,&mem[start_addr],size);
        return true;
    }
    else return false;
}/*}}}*/
bool axi4_mem::write(off_t start_addr, size_t size, const uint8_t* buffer) {/*{{{*/
    if (start_addr + size <= mem_size) {
        memcpy(&mem[start_addr],buffer,size);
        return true;
    }
    else return false;
}/*}}}*/
void axi4_mem::load_binary(const char *init_file, uint64_t start_addr) {/*{{{*/
    std::ifstream file(init_file,std::ios::in | std::ios::binary | std::ios::ate);
    size_t file_size = file.tellg();
    file.seekg(std::ios_base::beg);
    if (start_addr >= mem_size || file_size > mem_size - start_addr) {
        std::cerr << "memory size is not big enough for init file." << std::endl;
        file_size = mem_size;
    }
    file.read((char*)mem+start_addr,file_size);
}/*}}}*/
axi_resp axi4_mem::do_read(uint64_t start_addr, uint64_t size, uint8_t* buffer) {/*{{{*/
    if (start_addr + size <= mem_size) {
        memcpy(buffer,&mem[start_addr],size);
        return RESP_OKEY;
    }
    else return RESP_DECERR;
}/*}}}*/
axi_resp axi4_mem::do_write(uint64_t start_addr, uint64_t size, const uint8_t* buffer) {/*{{{*/
    if (start_addr + size <= mem_size) {
        memcpy(&mem[start_addr],buffer,size);
        return RESP_OKEY;
    }
    else return RESP_DECERR;
}/*}}}*/
