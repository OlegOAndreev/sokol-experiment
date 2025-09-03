#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/io.h"


// One mip level of WAD texture.
struct WAD3MiptexLevel {
    // RGBA data (alpha is always 255).
    std::vector<uint8_t> data;
};

// WAD texture + mipmaps.
struct WAD3Miptex {
    static constexpr int NUM_LEVELS = 4;

    // Texture name.
    std::string name;
    // Texture dimensions.
    uint32_t width;
    uint32_t height;
    WAD3MiptexLevel mipmaps[NUM_LEVELS];
};

// Parser for WAD files from HL1.
struct WAD3Parser {
    // Parse file, set valid and other fields. Return false if parsing failed (valid will be false as well).
    bool parse(const FileContents& file);

    // False if the parse() was not called or returned an error.
    bool valid = false;
    // File name.
    std::string name;
    // Parsed mip textures.
    std::vector<WAD3Miptex> miptexs;
};
