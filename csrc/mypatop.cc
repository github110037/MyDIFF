#include "PaddrInterface.hpp"
#include <list>
class PaddrTop: public PaddrInterface{
    private:
        PaddrInterface* pmem;
        std::list<PaddrInterface> devices;
    public:
        bool add_main_mem(PaddrInterface pmem);
        bool add_sub_device(PaddrInterface device);
        bool do_read (uint64_t start_addr, uint64_t size, uint8_t* buffer);
        bool do_write(uint64_t start_addr, uint64_t size, const uint8_t* buffer);
};
class MyDevice: public PaddrInterface{

};
