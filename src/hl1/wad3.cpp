#include "wad3.h"

#include <cstdint>
#include <cstring>

#include "common/io.h"
#include "common/slog.h"

// See https://twhl.info/wiki/page/Specification:_WAD3 for details
namespace {

const uint8_t MIPTEX_FILE_TYPE = 0x43;

struct WAD3Header {
    char magic[4];
    uint32_t num_dirs;
    uint32_t dir_offset;
};

struct WAD3DirEntry {
    uint32_t entry_offset;
    uint32_t disk_size;
    uint32_t entry_size;
    uint8_t file_type;
    bool compressed;
    int16_t padding;
    char texture_name[16];
};

struct WAD3RawMiptexHeader {
    char texture_name[16];
    uint32_t width;
    uint32_t height;
    uint32_t mip_offsets[4];
};

struct WAD3RawMiptexTrailer {
    int16_t colors_used;
    uint8_t palette[256 * 3];
};

bool parse_miptex(const FileContents& file, const WAD3DirEntry& entry, WAD3Miptex* miptex) {
    if (entry.entry_size < sizeof(WAD3RawMiptexHeader)) {
        SLOG_ERROR("%s: Entry size for %s must be at least %zu, is %d", file.name.c_str(), entry.texture_name,
                   sizeof(WAD3RawMiptexHeader), int(entry.entry_size));
        return false;
    }

    WAD3RawMiptexHeader header = {};
    if (!file.read_at(entry.entry_offset, &header)) {
        SLOG_ERROR("%s: Entry %s out of bounds", file.name.c_str(), entry.texture_name);
        return false;
    }
    if (header.width == 0 || header.height == 0 || (header.width % 16) != 0 || (header.height % 16) != 0) {
        SLOG_ERROR("%s: Invalid texture dimensions %ux%u for %s", file.name.c_str(), header.width, header.height,
                   entry.texture_name);
        return false;
    }

    size_t trailer_offset = entry.entry_offset + header.mip_offsets[3] + header.width * header.height / 64;
    WAD3RawMiptexTrailer trailer = {};
    if (!file.read_at(trailer_offset, &trailer)) {
        SLOG_ERROR("%s: Entry %s out of bounds", file.name.c_str(), entry.texture_name);
        return false;
    }

    if (trailer.colors_used != 256) {
        SLOG_ERROR("%s: Invalid number of colors (%d) in palette for %s", file.name.c_str(), trailer.colors_used,
                   entry.texture_name);
        return false;
    }

    miptex->name = std::string(header.texture_name, strnlen(header.texture_name, 16));
    miptex->width = header.width;
    miptex->height = header.height;

    uint32_t width = header.width;
    uint32_t height = header.height;
    for (int mip_level = 0; mip_level < WAD3Miptex::NUM_LEVELS; mip_level++) {
        uint32_t mip_offset = header.mip_offsets[mip_level];
        uint32_t mip_size = width * height;
        uint32_t absolute_offset = entry.entry_offset + mip_offset;
        if (absolute_offset + mip_size > file.contents.size()) {
            SLOG_ERROR("%s: Mipmap %d for %s out of bounds", file.name.c_str(), mip_level, entry.texture_name);
            return false;
        }

        miptex->mipmaps[mip_level].data.resize(mip_size * 4);
        uint8_t* data_ptr = miptex->mipmaps[mip_level].data.data();
        const uint8_t* contents_ptr = file.contents.data();
        for (size_t i = 0; i < mip_size; i++) {
            uint8_t color = contents_ptr[absolute_offset + i];
            uint8_t r = trailer.palette[color * 3];
            uint8_t g = trailer.palette[color * 3 + 1];
            uint8_t b = trailer.palette[color * 3 + 2];
            data_ptr[i * 4] = r;
            data_ptr[i * 4 + 1] = g;
            data_ptr[i * 4 + 2] = b;
            data_ptr[i * 4 + 3] = 255;
        }

        width /= 2;
        height /= 2;
    }

    return true;
}

bool parse_header(const FileContents& file, WAD3Header* header) {
    if (!file.read_at(0, header)) {
        SLOG_ERROR("%s: Insufficient data length for header", file.name.c_str());
        return false;
    }
    if (memcmp(header->magic, "WAD3", 4) != 0) {
        SLOG_ERROR("%s: Invalid magic number (expected 'WAD3')", file.name.c_str());
        return false;
    }
    return true;
}

bool process_directory(const FileContents& file, const WAD3Header& header, std::vector<WAD3Miptex>* miptexs) {
    for (size_t i = 0; i < header.num_dirs; i++) {
        WAD3DirEntry entry = {};
        if (!file.read_at(header.dir_offset + i * sizeof(WAD3DirEntry), &entry)) {
            SLOG_ERROR("%s: Insufficient data length for directory", file.name.c_str());
            return false;
        }

        if (entry.file_type != MIPTEX_FILE_TYPE) {
            continue;
        }
        if (entry.compressed) {
            SLOG_ERROR("%s: Got compressed entry %s, skipping", file.name.c_str(), entry.texture_name);
            continue;
        }

        WAD3Miptex miptex = {};
        if (!parse_miptex(file, entry, &miptex)) {
            continue;
        }

        miptexs->push_back(std::move(miptex));
    }

    return true;
}

}  // namespace

bool WAD3Parser::parse(const FileContents& file) {
    valid = false;
    name = get_path_filename(file.name.c_str());
    miptexs.clear();

    WAD3Header header = {};
    if (!parse_header(file, &header)) {
        return false;
    }
    if (!process_directory(file, header, &miptexs)) {
        return false;
    }

    valid = true;
    return true;
}
