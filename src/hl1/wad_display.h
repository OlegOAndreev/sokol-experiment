#pragma once

#include <sokol_gfx.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "wad3.h"

struct WAD3Display {
    void set_loading(bool loading);

    void add_wad(const WAD3Parser& wad);

    void render();

    struct TextureEntry {
        std::string name;
        sg_image image;
        uint32_t width;
        uint32_t height;
    };

    struct WADEntry {
        std::string name;
        std::vector<TextureEntry> textures;
    };

    bool loading = true;
    std::vector<WADEntry> wads;
    int selected_wad_index = -1;
    int selected_texture_index = -1;
    std::string filter_text;
};
