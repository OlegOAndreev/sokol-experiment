#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "common/io.h"

struct WAD3MiptexLevel {
    // RGB data
    std::vector<uint8_t> data;
};

struct WAD3Miptex {
    static constexpr int NUM_LEVELS = 4;

    std::string name;
    uint32_t width;
    uint32_t height;
    WAD3MiptexLevel mipmaps[NUM_LEVELS];
};

struct WAD3Parser {
    bool parse(const FileContents& file);

    bool valid = false;
    std::string name;
    std::vector<WAD3Miptex> miptexs;
};
