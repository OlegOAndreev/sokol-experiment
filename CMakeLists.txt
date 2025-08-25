cmake_minimum_required(VERSION 3.25)

project(sokol-experiment)

include(cmake/sokol.cmake)

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  # See https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
  # and https://libcxx.llvm.org/Hardening.html
  target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall
        -Wextra
        -Wno-unused-parameter
        -D_FORTIFY_SOURCE=2
        -D_GLIBCXX_ASSERTIONS
        -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST
        -fno-delete-null-pointer-checks
        -fno-strict-overflow
        -fstack-protector
        -ftrivial-auto-var-init=zero
        -fsanitize=address
        # -fsanitize=thread
        -fsanitize=undefined
  )
  target_link_options(${PROJECT_NAME} PRIVATE
        -fsanitize=address
        # -fsanitize=thread
        -fsanitize=undefined
  )
endif()

add_sokol_options(${PROJECT_NAME})
if(APPLE)
  enable_language(OBJCXX)
  target_sources(${PROJECT_NAME} PRIVATE src/sokol.mm)
else()
  target_sources(${PROJECT_NAME} PRIVATE src/sokol.cpp)
endif()

compile_glsl(${PROJECT_NAME} src/shaders/quad_shader.glsl)

file(GLOB IMGUI_SOURCES vendor/imgui/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE
        src/main.cpp

        src/common/io.cpp
        src/common/thread_pool.cpp

        src/hl1/wad3.cpp
        src/hl1/wad_display.cpp

        ${IMGUI_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        src
        vendor/imgui
        vendor/sokol
)
