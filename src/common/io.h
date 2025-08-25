#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// File contents + file name.
struct FileContents {
    std::string name;
    std::vector<uint8_t> contents;

    // Copy data from offset into dst or return false if the reads are out of bounds.
    template <typename T>
    bool read_at(size_t offset, T* dst) const {
        if (offset + sizeof(T) > contents.size()) {
            return false;
        }
        memcpy(dst, &contents[offset], sizeof(T));
        return true;
    }
};

// Return directory part of path or empty string. Path is assumed to have '/' delimiters.
std::string get_path_directory(const char* path);
// Return file part of path. Path is assumed to have '/' delimiters.
std::string get_path_filename(const char* path);
// Join two paths. If the second path is absolute, return second path. Paths are assumed to have '/' delimiters.
std::string join_paths(const char* path1, const char* path2);

// Read contents of file or return false (out will be cleared on error).
bool read_path_contents(const char* path, FileContents* out);

// Read contents of file or return false (out will NOT be cleared on error).
bool read_path_lines(const char* path, std::vector<std::string>* out);
