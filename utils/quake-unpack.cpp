#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "common/io.h"

namespace {

#pragma pack(push, 1)
struct PakHeader {
    char id[4];      // Should be "PACK"
    uint32_t offset; // Index to the beginning of the file table
    uint32_t size;   // Size of the file table
};

struct PakFileEntry {
    char name[56];   // Null-terminated string with path
    uint32_t offset; // Offset to the beginning of this file's contents
    uint32_t size;   // Size of this file
};
#pragma pack(pop)

bool validate_pak_header(const PakHeader& header) {
    return std::memcmp(header.id, "PACK", 4) == 0;
}

bool create_directory_for_file(const std::string& filepath) {
    std::string dir = path_get_directory(filepath.c_str());
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            printf("Failed to create directory '%s': %s\n", dir.c_str(), ec.message().c_str());
            return false;
        }
    }
    return true;
}

bool write_file_contents(const std::string& filepath, const uint8_t* data, size_t size) {
    FILE* f = fopen(filepath.c_str(), "wb");
    if (f == nullptr) {
        printf("Failed to open file for writing: %s\n", filepath.c_str());
        return false;
    }
    
    size_t bytes_written = fwrite(data, 1, size, f);
    fclose(f);
    
    if (bytes_written != size) {
        printf("Failed to write all data to file: %s (wrote %zu of %zu bytes)\n", 
               filepath.c_str(), bytes_written, size);
        return false;
    }
    
    return true;
}

bool unpack_pak_file(const std::string& pak_path, const std::string& output_dir) {
    // Read the entire .pak file
    FileContents pak_contents;
    if (!file_read_contents(pak_path.c_str(), pak_contents)) {
        printf("Failed to read .pak file: %s\n", pak_path.c_str());
        return false;
    }
    
    // Read and validate header
    PakHeader header;
    if (!pak_contents.read_at(0, header)) {
        printf("Failed to read .pak header\n");
        return false;
    }
    
    if (!validate_pak_header(header)) {
        printf("Invalid .pak file: header ID is not 'PACK'\n");
        return false;
    }
    
    // Calculate number of files
    size_t file_count = header.size / sizeof(PakFileEntry);
    if (header.size % sizeof(PakFileEntry) != 0) {
        printf("Warning: File table size is not a multiple of file entry size\n");
    }
    
    printf("Found %zu files in .pak archive\n", file_count);
    
    // Read file entries
    std::vector<PakFileEntry> file_entries(file_count);
    for (size_t i = 0; i < file_count; ++i) {
        size_t entry_offset = header.offset + i * sizeof(PakFileEntry);
        if (!pak_contents.read_at(entry_offset, file_entries[i])) {
            printf("Failed to read file entry %zu\n", i);
            return false;
        }
    }
    
    // Extract files
    size_t success_count = 0;
    for (size_t i = 0; i < file_count; ++i) {
        const auto& entry = file_entries[i];
        
        // Ensure the filename is null-terminated
        std::string filename(entry.name, strnlen(entry.name, sizeof(entry.name)));
        
        // Skip empty filenames
        if (filename.empty()) {
            printf("Warning: Skipping file with empty name at index %zu\n", i);
            continue;
        }
        
        // Validate file data bounds
        if (entry.offset + entry.size > pak_contents.contents.size()) {
            printf("Warning: File '%s' has invalid offset/size (offset: %u, size: %u, pak size: %zu)\n",
                   filename.c_str(), entry.offset, entry.size, pak_contents.contents.size());
            continue;
        }
        
        // Create output path
        std::string output_path = path_join(output_dir.c_str(), filename.c_str());
        
        // Create directory structure
        if (!create_directory_for_file(output_path)) {
            continue;
        }
        
        // Write file
        const uint8_t* file_data = &pak_contents.contents[entry.offset];
        if (write_file_contents(output_path, file_data, entry.size)) {
            printf("Extracted: %s\n", filename.c_str());
            success_count++;
        } else {
            printf("Failed to extract: %s\n", filename.c_str());
        }
    }
    
    printf("Successfully extracted %zu of %zu files\n", success_count, file_count);
    return success_count > 0;
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

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string pak_path = argv[1];
    std::string output_dir = argv[2];

    if (!make_directories(output_dir.c_str())) {
        return 1;
    }
    
    // // Create output directory if it doesn't exist
    // std::error_code ec;
    // if (!std::filesystem::exists(output_dir)) {
    //     if (!std::filesystem::create_directories(output_dir, ec)) {
    //         printf("Failed to create output directory '%s': %s\n", output_dir.c_str(), ec.message().c_str());
    //         return 1;
    //     }
    // }
    
    // if (!unpack_pak_file(pak_path, output_dir)) {
    //     printf("Failed to unpack .pak file\n");
    //     return 1;
    // }
    
    return 0;
}
