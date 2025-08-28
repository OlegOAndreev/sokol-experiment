#include "io.h"

#include <sokol_slog.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "defer.h"


namespace {

bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

size_t trim_end(const char* line) {
    size_t n = strlen(line);
    while (n > 0 && is_space(line[n - 1])) {
        n--;
    }
    return n;
}

}  // namespace

std::string path_get_directory(const char* path) {
    const char* last = strrchr(path, '/');
    if (last == nullptr) {
        return "";
    }
    return std::string(path, last);
}

std::string path_get_filename(const char* path) {
    const char* last = strrchr(path, '/');
    if (last == nullptr) {
        return path;
    }
    return last;
}

std::string path_join(const char* path1, const char* path2) {
    if (strlen(path2) == 0) {
        return path1;
    }
    if (path2[0] == '/') {
        return path2;
    }
    std::string result = path1;
    if (!result.empty() && result.back() != '/') {
        result += '/';
    }
    result += path2;
    return result;
}


bool file_read_contents(const char* path, FileContents& out) {
    out.name = path;
    out.contents.clear();

    FILE* f = fopen(path, "rb");
    if (f == nullptr) {
        SLOG_ERROR("Could not open '%s'", path);
        return false;
    }
    DEFER(fclose(f));

    if (fseek(f, 0, SEEK_END) != 0) {
        SLOG_ERROR("Could not seek to end of '%s'", path);
        return false;
    }
    long file_size = ftell(f);
    if (file_size < 0) {
        SLOG_ERROR("Could not get file size of '%s'", path);
        return false;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        SLOG_ERROR("Could not seek to start of '%s'", path);
        return false;
    }

    out.contents.resize(size_t(file_size));
    size_t bytes_read = fread(out.contents.data(), 1, size_t(file_size), f);
    if (bytes_read != (size_t)file_size) {
        SLOG_ERROR("Tried to read %ld, but got only %zu bytes from '%s'", file_size, bytes_read, path);
        out.contents.clear();
        return false;
    }

    return true;
}

bool file_read_lines(const char* path, std::vector<std::string>& out) {
    FILE* f = fopen(path, "rb");
    if (f == nullptr) {
        SLOG_ERROR("Could not open '%s'", path);
        return false;
    }
    DEFER(fclose(f));

    char buf[10000];
    while (fgets(buf, sizeof(buf), f) != nullptr) {
        out.emplace_back(buf, trim_end(buf));
    }
    return true;
}
