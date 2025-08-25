#pragma once

#include <sokol_gfx.h>

#include "bsp.h"

struct BSPDisplay {
    // Initialize BSPDisplay.
    void init();

    // Set BSP file to display.
    void set_bsp(const BSPParser& wad);

    // ????
    void render();
};
