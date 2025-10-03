#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "common/defer.h"
#include "common/io.h"

namespace {

// See https://quakewiki.org/wiki/.pak for format description
struct PakHeader {
    char id[4];
    uint32_t offset;
    uint32_t size;
};

struct PakFileEntry {
    char name[56];
    uint32_t offset;
    uint32_t size;
};

bool create_directory_for_file(const std::string& filepath) {
    std::string dir = path_get_directory(filepath.c_str());
    if (!dir.empty()) {
        return make_directories(dir.c_str());
    }
    return true;
}

bool unpack_pak_file(const char* pak_path, const char* output_dir) {
    FILE* pak_f = fopen(pak_path, "rb");
    if (pak_f == nullptr) {
        printf("Failed to open .pak file: %s\n", pak_path);
        return false;
    }
    DEFER(fclose(pak_f));

    PakHeader header{};
    if (fread(&header, sizeof(PakHeader), 1, pak_f) != 1) {
        printf("Failed to read .pak header\n");
        return false;
    }
    if (memcmp(header.id, "PACK", 4) != 0) {
        printf("Invalid .pak file: header ID is not 'PACK'\n");
        return false;
    }

    // Calculate number of files
    size_t file_count = header.size / sizeof(PakFileEntry);
    if (header.size % sizeof(PakFileEntry) != 0) {
        printf("Warning: File table size is not a multiple of file entry size\n");
    }

    printf("Found %zu files in .pak archive\n", file_count);

    std::vector<PakFileEntry> file_entries(file_count);
    if (fseek(pak_f, header.offset, SEEK_SET) != 0) {
        printf("Failed to seek to file table\n");
        return false;
    }

    if (fread(file_entries.data(), sizeof(PakFileEntry), file_count, pak_f) != file_count) {
        printf("Failed to read file entries\n");
        return false;
    }

    for (const PakFileEntry& entry : file_entries) {
        std::string output_file = path_join(output_dir, entry.name);
        if (!create_directory_for_file(output_file)) {
            return false;
        }

        if (fseek(pak_f, entry.offset, SEEK_SET) != 0) {
            printf("Failed to seek to file data for: %s\n", entry.name);
            return false;
        }

        std::vector<uint8_t> file_data(entry.size);
        if (fread(file_data.data(), 1, entry.size, pak_f) != entry.size) {
            printf("Failed to read file data for: %s\n", entry.name);
            return false;
        }

        if (!file_write_contents(output_file.c_str(), file_data.data(), entry.size)) {
            return false;
        }
        printf("Extracted: %s\n", entry.name);
    }

    printf("Successfully extracted %zu files\n", file_count);
    return true;
}

void print_usage(const char* program_name) {
    printf("Usage: %s <pak_file> <output_directory>\n", program_name);
    printf("\n");
    printf("Extracts all files from a Quake .pak archive into the specified directory.\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  pak_file         Path to the .pak file to extract\n");
    printf("  output_directory Directory where extracted files will be placed\n");
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* pak_path = argv[1];
    const char* output_dir = argv[2];

    if (!make_directories(output_dir)) {
        return 1;
    }

    if (!unpack_pak_file(pak_path, output_dir)) {
        printf("Failed to unpack .pak file\n");
        return 1;
    }

    return 0;
}
