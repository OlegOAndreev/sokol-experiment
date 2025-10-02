#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// File byte contents + file name.
struct FileContents {
    std::string name;
    std::vector<uint8_t> contents;

    // Copy data from offset into dst or return false if the reads are out of bounds.
    template <typename T>
    bool read_at(size_t offset, T& dst) const {
        if (offset + sizeof(T) > contents.size()) {
            return false;
        }
        memcpy(&dst, &contents[offset], sizeof(T));
        return true;
    }
};


// Return directory part of path or empty string. Path is assumed to have '/' delimiters.
std::string path_get_directory(const char* path);
// Return file part of path. Path is assumed to have '/' delimiters.
std::string path_get_filename(const char* path);
// Join two paths. If the second path is absolute, return second path. Paths are assumed to have '/' delimiters.
std::string path_join(const char* path1, const char* path2);

// Create directory and all its parents if required. Return true on success.
bool make_directories(const char* path);

// Read contents of file or return false (out is cleared on error).
bool file_read_contents(const char* path, FileContents& out);
// Read contents of file or return false (out is NOT cleared on error).
bool file_read_lines(const char* path, std::vector<std::string>& out);
