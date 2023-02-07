#include "common.h"
#include <cstdio>
#include "diff_sim.hpp"

FILE * log_dt = nullptr;
void init_log(const char* filename){
    if (filename != nullptr) {
        log_dt = fopen(filename, "w");
    }
    Log("DiffTest Log is written to %s", log_dt ? filename : "stdout");
}
