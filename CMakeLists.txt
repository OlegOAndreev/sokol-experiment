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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  # See https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
  # and https://libcxx.llvm.org/Hardening.html
  target_compile_options(${PROJECT_NAME} PRIVATE 
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
    target_compile_options(${PROJECT_NAME} PRIVATE
        -D_FORTIFY_SOURCE=2
        -D_GLIBCXX_ASSERTIONS)
  endif()

  if(ENABLE_ASAN AND NOT(EMSCRIPTEN))
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -fsanitize=address
        -fsanitize=undefined
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        -fsanitize=address
        -fsanitize=undefined
    )
  endif()
  if(ENABLE_TSAN AND NOT(EMSCRIPTEN))
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -fsanitize=thread
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        -fsanitize=thread
    )
  endif()
endif()

target_add_sokol(${PROJECT_NAME} ${SOKOL_DEBUG} false)

compile_glsl(${PROJECT_NAME} src/shaders/quad_shader.glsl)

set(COMMON_SOURCES
        src/common/io.cpp
        src/common/thread.cpp
)
set(HL1_SOURCES
        src/hl1/bsp.cpp
        src/hl1/bsp_display.cpp
        src/hl1/wad3.cpp
        src/hl1/wad_display.cpp
)
file(GLOB IMGUI_SOURCES vendor/imgui/*.cpp)

target_sources(${PROJECT_NAME} PRIVATE
        src/main.cpp
        ${COMMON_SOURCES}
        ${HL1_SOURCES}
        ${IMGUI_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        src
        vendor/imgui
)

# # DEBUG DEBUG DEBUG
# target_link_options(${PROJECT_NAME} PRIVATE /opt/homebrew/lib/libtcmalloc.dylib)
# # DEBUG DEBUG DEBUG

if(EMSCRIPTEN)
  # target_link_options(${PROJECT_NAME} PRIVATE -sASSERTIONS -sNO_EXIT_RUNTIME --memoryprofiler)
endif()

if(BUILD_TESTS AND NOT(EMSCRIPTEN))
  add_executable(tests)

  target_add_sokol_for_tests(tests)
  target_include_directories(tests PRIVATE
        src
  )

  file(GLOB TEST_SOURCES src/**/tests/*_test.cpp)
  target_sources(tests PRIVATE
        ${TEST_SOURCES}
        ${COMMON_SOURCES}
        ${IMGUI_SOURCES}
  )

  add_subdirectory(vendor/snitch)
  target_link_libraries(tests PRIVATE snitch::snitch)
endif()
