#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/io.h"


// BSP parser for HL1.
struct BSPParser {
    // Parse file, set valid and other fields. Return false if parsing failed (valid will be false as well).
    bool parse(const FileContents& file);

    // False if the parse() was not called or returned an error.
    bool valid = false;
    // File name.
    std::string name;
};
