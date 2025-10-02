#include "io.h"

#include <sys/stat.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "defer.h"
#include "slog.h"


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
    if (last == path) {
        return "/";
    }
    return std::string(path, last);
}

std::string path_get_filename(const char* path) {
    const char* last = strrchr(path, '/');
    if (last == nullptr) {
        return path;
    }
    return last + 1;
}

std::string path_join(const char* path1, const char* path2) {
    if (path2[0] == 0) {
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

static bool make_directories_impl(char* path) {
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return true;
    }
    if (errno != ENOENT) {
        SLOG_ERROR("Could not create directory '%s': %s", path, strerror(errno));
        return false;
    }

    // Recursively create parent directories and retry creating the directory.
    char* last_slash = strrchr(path, '/');
    if (last_slash != nullptr && last_slash != path) {
        *last_slash = '\0';
        if (!make_directories_impl(path)) {
            return false;
        }
        *last_slash = '/';

        if (mkdir(path, 0755) == 0 || errno == EEXIST) {
            return true;
        }
    }
    SLOG_ERROR("Could not create directory '%s': %s", path, strerror(errno));
    return false;
}

bool make_directories(const char* path) {
    std::string tmp_path = path;
    if (tmp_path.empty()) {
        return true;
    }
    if (tmp_path.back() == '/') {
        tmp_path.pop_back();
    }
    return make_directories_impl(tmp_path.data());
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
