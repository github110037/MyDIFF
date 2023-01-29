#ifndef __DPIC_HPP__
#define __DPIC_HPP__
#include <stdint.h>
uint32_t dpi_regfile(uint8_t num);
uint8_t dpi_retire();
uint32_t dpi_retirePC();
uint32_t dpi_cp0_count();
#endif // !__DPIC_HPP__
