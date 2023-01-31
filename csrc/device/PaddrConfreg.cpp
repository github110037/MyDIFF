#include "PaddrInterface.hh"
PaddrConfreg::PaddrConfreg(bool simulation) {
    timer = 0;
    memset(cr,0,sizeof(cr));
    led = 0;
    led_rg0 = 0;
    led_rg1 = 0;
    num = 0;
    simu_flag = simulation ? 0xffffffffu : 0;
    io_simu = 0;
    open_trace = 1;
    num_monitor = 1;
    virtual_uart = 0;
    set_switch(0);
}
void PaddrConfreg::tick() {
    timer ++;
}
bool PaddrConfreg::do_read (word_t addr, size_wstrb info, word_t* data) {
    confreg_read ++;
    assert(info.size == 4);
    switch (addr) {
        case CR0_ADDR:
            *(uint32_t*)data = cr[0];
            break;
        case CR1_ADDR:
            *(uint32_t*)data = cr[1];
            break;
        case CR2_ADDR:
            *(uint32_t*)data = cr[2];
            break;
        case CR3_ADDR:
            *(uint32_t*)data = cr[3];
            break;
        case CR4_ADDR:
            *(uint32_t*)data = cr[4];
            break;
        case CR5_ADDR:
            *(uint32_t*)data = cr[5];
            break;
        case CR6_ADDR:
            *(uint32_t*)data = cr[6];
            break;
        case CR7_ADDR:
            *(uint32_t*)data = cr[7];
            break;
        case LED_ADDR:
            *(uint32_t *)data = led;
            break;
        case LED_RG0_ADDR:
            *(uint32_t *)data = led_rg0;
            break;
        case LED_RG1_ADDR:
            *(uint32_t *)data = led_rg1;
            break;
        case NUM_ADDR:
            *(uint32_t *)data = num;
            break;
        case SWITCH_ADDR:
            *(uint32_t *)data = switch_data;
            break;
        case BTN_KEY_ADDR:
            *(uint32_t *)data = 0;
            break;
        case BTN_STEP_ADDR:
            *(uint32_t *)data = 0;
            break;
        case SW_INTER_ADDR:
            *(uint32_t *)data = switch_inter_data;
            break;
        case TIMER_ADDR:
            *(uint32_t *)data = timer;
            break;
        case SIMU_FLAG_ADDR:
            *(uint32_t *)data = simu_flag;
            break;
        case IO_SIMU_ADDR:
            *(uint32_t *)data = io_simu;
            break;
        case VIRTUAL_UART_ADDR:
            *(uint32_t *)data = virtual_uart;
            break;
        case OPEN_TRACE_ADDR:
            *(uint32_t *)data = open_trace;
            break;
        case NUM_MONITOR_ADDR:
            *(uint32_t *)data = num_monitor;
            break;
        default:
            *(uint32_t *)data = 0;
            Warn("read confreg not exist address: " FMT_WORD_X ,addr);
            break;
    }
    return true;
}
bool PaddrConfreg::do_write(word_t addr, size_wstrb info, const word_t data){
    confreg_write ++;
    assert(info.size == 4 || (info.size == 1 && addr == VIRTUAL_UART_ADDR));
    switch (addr) {
        case CR0_ADDR:
            cr[0] = data;
            break;
        case CR1_ADDR:
            cr[1] = data;
            break;
        case CR2_ADDR:
            cr[2] = data;
            break;
        case CR3_ADDR:
            cr[3] = data;
            break;
        case CR4_ADDR:
            cr[4] = data;
            break;
        case CR5_ADDR:
            cr[5] = data;
            break;
        case CR6_ADDR:
            cr[6] = data;
            break;
        case CR7_ADDR:
            cr[7] = data;
            break;
        case TIMER_ADDR:
            timer = data;
            break;
        case IO_SIMU_ADDR:
            io_simu = (((data) & 0xffff) << 16) | ((data) >> 16);
            break;
        case OPEN_TRACE_ADDR: 
            open_trace = (data) != 0;
            break;
        case NUM_MONITOR_ADDR:
            num_monitor = (data) & 1;
            break;
        case VIRTUAL_UART_ADDR:
            virtual_uart = (data & 0xff);
            uart_queue.push(virtual_uart);
            break;
        case NUM_ADDR:
            num = data;
            break;
        default:
            Warn("write confreg not exist address: " FMT_WORD_X ,addr);
            break;
    }
    return true;
}
void PaddrConfreg::set_switch(uint8_t value) {
    switch_data = value ^ 0xf;
    switch_inter_data = 0;
    for (int i=0;i<=7;i++) {
        if ( ((value >> i) & 1) == 0) {
            switch_inter_data |= 2<<(2*i);
        }
    }
}
bool PaddrConfreg::has_uart() {
    return !uart_queue.empty();
}
char PaddrConfreg::get_uart() {
    char res = uart_queue.front();
    uart_queue.pop();
    return res;
}
uint32_t PaddrConfreg::get_num() {
    return num;
}


