#pragma once

#include <sokol_gfx.h>

#include <cstdint>
#include <string>
#include <vector>

#include "wad3.h"

// Display for WAD files from HL1.
struct WAD3Display {
    // Add a new WAD file to display.
    void add_wad(const WAD3Parser& wad);

    // Render a new ImGui window. Must be called after ImGui::Frame().
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
    bool scale_image = false;
};
