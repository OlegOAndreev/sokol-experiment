#pragma once

#include <sokol_gfx.h>

#include <cstdint>
#include <string>
#include <vector>

#include "common/struct.h"
#include "wad3.h"


// Display for WAD files from HL1.
struct WAD3Display {
    // Add a new WAD file to display.
    void add_wad(const WAD3Parser& wad);

    // Render a new ImGui window. Must be called after ImGui::Frame().
    void render();

    // Clears the resources.
    void destroy();

    struct TextureEntry {
        std::string name;
        sg_image image = {0};
        sg_view image_view = {0};
        uint32_t width = 0;
        uint32_t height = 0;

        // Clears the resources.
        void destroy();
    };

    struct WADEntry {
        std::string name;
        std::vector<TextureEntry> textures;

        // Clears the resources.
        void destroy();
    };

    bool loading = true;
    std::vector<WADEntry> wads;
    int selected_wad_index = -1;
    int selected_texture_index = -1;
    std::string filter_text;
    bool scale_image = false;
};
