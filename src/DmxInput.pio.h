// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// -------- //
// DmxInput //
// -------- //

#define DmxInput_wrap_target 1
#define DmxInput_wrap 6

static const uint16_t DmxInput_program_instructions[] = {
    0x202a, //  0: wait   0 pin, 10                  
            //     .wrap_target
    0x20aa, //  1: wait   1 pin, 10                  
    0x400a, //  2: in     pins, 10                   
    0xe021, //  3: set    x, 1                       
    0x4022, //  4: in     x, 2                       
    0x4076, //  5: in     null, 22                   
    0x8020, //  6: push   block                      
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program DmxInput_program = {
    .instructions = DmxInput_program_instructions,
    .length = 7,
    .origin = -1,
};

static inline pio_sm_config DmxInput_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + DmxInput_wrap_target, offset + DmxInput_wrap);
    return c;
}
#endif

