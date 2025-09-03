#include "common/io.h"

#include <snitch/snitch.hpp>
#include <string>


TEST_CASE("path_get_directory") {
    SECTION("path with directory") {
        CHECK(path_get_directory("/home/user/file.txt") == "/home/user");
        CHECK(path_get_directory("/usr/local/bin/app") == "/usr/local/bin");
        CHECK(path_get_directory("/root/") == "/root");
    }

    SECTION("path with single directory") {
        CHECK(path_get_directory("/dir/file") == "/dir");
        CHECK(path_get_directory("/file") == "/");
    }

    SECTION("filename only (no directory)") {
        CHECK(path_get_directory("file.txt") == "");
    }

    SECTION("empty path") {
        CHECK(path_get_directory("") == "");
    }

    SECTION("root directory") {
        CHECK(path_get_directory("/") == "/");
    }

    SECTION("relative paths") {
        CHECK(path_get_directory("dir/file.txt") == "dir");
        CHECK(path_get_directory("dir1/dir2/file") == "dir1/dir2");
        CHECK(path_get_directory("./dir/file") == "./dir");
        CHECK(path_get_directory("../dir/file") == "../dir");
        CHECK(path_get_directory("relative/path/") == "relative/path");
    }

    SECTION("paths with multiple slashes") {
        CHECK(path_get_directory("/home//user/file") == "/home//user");
        CHECK(path_get_directory("//root/dir/file") == "//root/dir");
    }
}

TEST_CASE("path_get_filename") {
    SECTION("path with filename") {
        CHECK(path_get_filename("/home/user/file.txt") == "file.txt");
        CHECK(path_get_filename("/usr/local/bin/app") == "app");
        CHECK(path_get_filename("/root/document.pdf") == "document.pdf");
    }

    SECTION("filename only (no directory)") {
        CHECK(path_get_filename("file.txt") == "file.txt");
        CHECK(path_get_filename("nodir") == "nodir");
        CHECK(path_get_filename("some_file.cpp") == "some_file.cpp");
    }

    SECTION("empty path") {
        CHECK(path_get_filename("") == "");
    }

    SECTION("path ending with slash") {
        CHECK(path_get_filename("/home/user/") == "");
        CHECK(path_get_filename("/root/") == "");
        CHECK(path_get_filename("dir/") == "");
        CHECK(path_get_filename("/") == "");
    }

    SECTION("relative paths") {
        CHECK(path_get_filename("dir/file.txt") == "file.txt");
        CHECK(path_get_filename("dir1/dir2/file") == "file");
        CHECK(path_get_filename("./dir/file.cpp") == "file.cpp");
        CHECK(path_get_filename("../dir/file") == "file");
    }

    SECTION("paths with multiple slashes") {
        CHECK(path_get_filename("/home//user/file") == "file");
        CHECK(path_get_filename("//root/dir/file.txt") == "file.txt");
    }

    SECTION("files with multiple dots") {
        CHECK(path_get_filename("/path/file.tar.gz") == "file.tar.gz");
        CHECK(path_get_filename("archive.backup.zip") == "archive.backup.zip");
        CHECK(path_get_filename("/dir/.hidden.file") == ".hidden.file");
    }

    SECTION("special filenames") {
        CHECK(path_get_filename("/path/.") == ".");
        CHECK(path_get_filename("/path/..") == "..");
        CHECK(path_get_filename("/path/.hidden") == ".hidden");
    }
}

TEST_CASE("path_join") {
    SECTION("join directory and filename") {
        CHECK(path_join("/home/user", "file.txt") == "/home/user/file.txt");
        CHECK(path_join("/usr/local", "bin") == "/usr/local/bin");
        CHECK(path_join("/root", "document.pdf") == "/root/document.pdf");
    }

    SECTION("join with directory ending in slash") {
        CHECK(path_join("/home/user/", "file.txt") == "/home/user/file.txt");
        CHECK(path_join("/usr/local/", "bin") == "/usr/local/bin");
        CHECK(path_join("/", "file") == "/file");
    }

    SECTION("join with absolute second path") {
        CHECK(path_join("/home/user", "/etc/config") == "/etc/config");
        CHECK(path_join("relative/path", "/absolute/path") == "/absolute/path");
        CHECK(path_join("", "/root/file") == "/root/file");
    }

    SECTION("join with empty second path") {
        CHECK(path_join("/home/user", "") == "/home/user");
        CHECK(path_join("/root/", "") == "/root/");
        CHECK(path_join("", "") == "");
    }

    SECTION("join with empty first path") {
        CHECK(path_join("", "file.txt") == "file.txt");
        CHECK(path_join("", "dir/file") == "dir/file");
    }

    SECTION("join relative paths") {
        CHECK(path_join("dir1", "dir2") == "dir1/dir2");
        CHECK(path_join("dir1/dir2", "file.txt") == "dir1/dir2/file.txt");
        CHECK(path_join("./dir", "file") == "./dir/file");
        CHECK(path_join("../dir", "subdir") == "../dir/subdir");
    }

    SECTION("join preserves multiple slashes") {
        CHECK(path_join("/home//user", "file") == "/home//user/file");
        CHECK(path_join("/home/user", "dir//file") == "/home/user/dir//file");
    }

    SECTION("join with single slash paths") {
        CHECK(path_join("/", "file") == "/file");
        CHECK(path_join("/", "/") == "/");
        CHECK(path_join("dir", "/") == "/");
    }
}

TEST_CASE("FileContents::read_at") {
    FileContents file_contents;
    file_contents.contents = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    SECTION("read single byte") {
        uint8_t value = 0;
        CHECK(file_contents.read_at(0, value));
        CHECK(value == 0x01);

        CHECK(file_contents.read_at(3, value));
        CHECK(value == 0x04);

        CHECK(file_contents.read_at(7, value));
        CHECK(value == 0x08);
    }

    SECTION("read uint16_t") {
        uint16_t value = 0;
        CHECK(file_contents.read_at(0, value));
        CHECK(value == 0x0201);

        CHECK(file_contents.read_at(2, value));
        CHECK(value == 0x0403);

        CHECK(file_contents.read_at(6, value));
        CHECK(value == 0x0807);
    }

    SECTION("read uint32_t") {
        uint32_t value = 0;
        CHECK(file_contents.read_at(0, value));
        CHECK(value == 0x04030201);  // Little endian: 0x01, 0x02, 0x03, 0x04

        CHECK(file_contents.read_at(4, value));
        CHECK(value == 0x08070605);  // Little endian: 0x05, 0x06, 0x07, 0x08
    }

    SECTION("read uint64_t") {
        uint64_t value = 0;
        CHECK(file_contents.read_at(0, value));
        CHECK(value == 0x0807060504030201ULL);  // All 8 bytes in little endian
    }

    SECTION("read custom struct") {
        struct TestStruct {
            uint16_t a = 0;
            uint8_t b = 0;
            uint8_t c = 0;
        };

        TestStruct value;
        CHECK(file_contents.read_at(0, value));
        CHECK(value.a == 0x0201);
        CHECK(value.b == 0x03);
        CHECK(value.c == 0x04);

        CHECK(file_contents.read_at(4, value));
        CHECK(value.a == 0x0605);
        CHECK(value.b == 0x07);
        CHECK(value.c == 0x08);
    }

    SECTION("out of bounds reads") {
        uint8_t byte_value = 0;
        uint16_t short_value = 0;
        uint32_t int_value = 0;
        uint64_t long_value = 0;

        // Reading beyond end of contents
        CHECK_FALSE(file_contents.read_at(8, byte_value));
        CHECK_FALSE(file_contents.read_at(7, short_value));
        CHECK_FALSE(file_contents.read_at(5, int_value));
        CHECK_FALSE(file_contents.read_at(1, long_value));

        // Edge case: exactly at boundary
        CHECK(file_contents.read_at(7, byte_value));
        CHECK(file_contents.read_at(6, short_value));
        CHECK(file_contents.read_at(4, int_value));
        CHECK(file_contents.read_at(0, long_value));
    }

    SECTION("empty file contents") {
        FileContents empty_file;
        empty_file.name = "empty";
        empty_file.contents.clear();

        uint8_t value = 0;
        CHECK_FALSE(empty_file.read_at(0, value));
    }

    SECTION("reading from single byte file") {
        FileContents single_byte_file;
        single_byte_file.name = "single";
        single_byte_file.contents = {0xFF};

        uint8_t byte_value = 0;
        uint16_t short_value = 0;

        CHECK(single_byte_file.read_at(0, byte_value));
        CHECK(byte_value == 0xFF);

        CHECK_FALSE(single_byte_file.read_at(0, short_value));
        CHECK_FALSE(single_byte_file.read_at(1, byte_value));
    }
}
