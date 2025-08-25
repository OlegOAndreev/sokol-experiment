#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

struct FileContents {
    std::string name;
    std::vector<uint8_t> contents;

    template <typename T>
    bool copy_from(size_t offset, T* dst) const {
        if (offset + sizeof(T) > contents.size()) {
            return false;
        }
        memcpy(dst, &contents[offset], sizeof(T));
        return true;
    }
};

std::string get_file_dir(const char* path);
std::string get_file_name(const char* path);

std::string join_paths(const char* path1, const char* path2);

bool read_path_contents(const char* path, FileContents* out);

bool read_path_lines(const char* path, std::vector<std::string>* out);
