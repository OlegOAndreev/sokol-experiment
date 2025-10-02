cmake_minimum_required(VERSION 3.25)

option(BUILD_TESTS "Build tests" ON)
option(SOKOL_DEBUG "Enable Sokol Debug" ON)
option(ENABLE_ASAN "Enable address and UB sanitizers" ON)
option(ENABLE_TSAN "Enable thread sanitizer" OFF)

project(sokol-experiment)
if(APPLE)
  enable_language(OBJCXX)
endif()

include(sokol/sokol.cmake)
include(scripts/macos/entitlements.cmake)

if(NOT CMAKE_BUILD_TYPE)
  if(EMSCRIPTEN)
    set(CMAKE_BUILD_TYPE Release)
  else()
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()
endif()

find_program(SCCACHE_PATH sccache)
if(SCCACHE_PATH)
  message(STATUS "Using sccache at ${SCCACHE_PATH}")
  set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PATH})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PATH})
endif()

# Compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_COMPILE_FLAGS "")
set(COMMON_LINK_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  # See https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
  # and https://libcxx.llvm.org/Hardening.html
  list(APPEND COMMON_COMPILE_FLAGS
      -Wall
      -Wextra
      -Wno-unused-parameter
      -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST
      -fno-delete-null-pointer-checks
      -fno-strict-overflow
      -fstack-protector
      -ftrivial-auto-var-init=zero
  )
  if(NOT APPLE)
    list(APPEND COMMON_COMPILE_FLAGS
        -D_FORTIFY_SOURCE=2
        -D_GLIBCXX_ASSERTIONS
    )
  endif()
  if(ENABLE_ASAN AND NOT(EMSCRIPTEN))
    list(APPEND COMMON_COMPILE_FLAGS 
        -fsanitize=address
        -fsanitize=undefined
    )
    list(APPEND COMMON_LINK_FLAGS
        -fsanitize=address
        -fsanitize=undefined
    )
  endif()
  if(ENABLE_TSAN AND NOT(EMSCRIPTEN))
    list(APPEND COMMON_COMPILE_FLAGS
        -fsanitize=thread
    )
    list(APPEND COMMON_LINK_FLAGS
        -fsanitize=thread
    )
  endif()
endif()

# Main executable
add_executable(sokol-experiment)
target_compile_options(sokol-experiment PRIVATE ${COMMON_COMPILE_FLAGS})
target_link_options(sokol-experiment PRIVATE ${COMMON_LINK_FLAGS})
target_add_sokol(sokol-experiment ${SOKOL_DEBUG})
target_add_macos_entitlements(sokol-experiment)
compile_glsl(sokol-experiment src/shaders/quad_shader.glsl)
set(COMMON_SOURCES
        src/common/io.cpp
        src/common/sync.cpp
        src/common/thread.cpp
)
set(HL1_SOURCES
        src/hl1/bsp.cpp
        src/hl1/bsp_display.cpp
        src/hl1/wad3.cpp
        src/hl1/wad_display.cpp
)
file(GLOB IMGUI_SOURCES vendor/imgui/*.cpp)
target_sources(sokol-experiment PRIVATE
        src/main.cpp
        ${COMMON_SOURCES}
        ${HL1_SOURCES}
        ${IMGUI_SOURCES}
)
target_include_directories(sokol-experiment PRIVATE
        src
        vendor/imgui
)
if(EMSCRIPTEN)
  target_link_options(sokol-experiment PRIVATE
        -sEXIT_RUNTIME=0
        -sSTACK_OVERFLOW_CHECK=1
        # -sASSERTIONS
        # --memoryprofiler
  )
endif()

# Utilities
add_executable(quake-unpack)
target_sources(quake-unpack PRIVATE
        utils/quake-unpack.cpp
        ${COMMON_SOURCES}
)
target_include_directories(quake-unpack PRIVATE src)
target_add_sokol_for_cli(quake-unpack)
target_compile_options(quake-unpack PRIVATE ${COMMON_COMPILE_FLAGS})
target_link_options(quake-unpack PRIVATE ${COMMON_LINK_FLAGS})

# Tests
if(BUILD_TESTS AND NOT(EMSCRIPTEN))
  add_executable(tests)

  target_add_sokol_for_tests(tests)
  target_add_macos_entitlements(tests)
  target_include_directories(tests PRIVATE
        src
  )

  set(TEST_SOURCES
        src/common/tests/bits_test.cpp
        src/common/tests/defer_test.cpp
        src/common/tests/io_test.cpp
        src/common/tests/queue_test.cpp
        src/common/tests/sync_test.cpp
        src/common/tests/thread_test.cpp
  )
  target_sources(tests PRIVATE
        src/tests_main.cpp
        ${TEST_SOURCES}
        ${COMMON_SOURCES}
        ${IMGUI_SOURCES}
  )

  target_compile_options(tests PRIVATE ${COMMON_COMPILE_FLAGS}
        -fsanitize=address
        -fsanitize=undefined
  )
  target_link_options(tests PRIVATE
        -fsanitize=address
        -fsanitize=undefined
  )

  add_subdirectory(vendor/doctest)
  target_link_libraries(tests PRIVATE doctest::doctest)
endif()
