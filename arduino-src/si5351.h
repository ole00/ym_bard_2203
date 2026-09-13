#pragma once

#include <stdint.h>
#define USE_SI5351_CLOCK 1

// Initialises Si5351, returns true on success.
bool Si5351_init();

// Sets 3 MHz clock on channel 0.
void Si5351_set_3_mhz();
