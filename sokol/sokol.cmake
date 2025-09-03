# Top-level CMakeLists.txt must include:
#   if(APPLE)
#     enable_language(OBJCXX)
#   endif()

set(THIS_SOKOL_DIR ${CMAKE_CURRENT_LIST_DIR})

if(NOT DEFINED SOKOL_SHDC_SLANG)
  set(SOKOL_SHDC_SLANG "glsl300es:glsl310es:metal_macos")
endif()

if(NOT DEFINED SOKOL_INCLUDE_DIR)
  set(SOKOL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vendor/sokol")
endif()

if(NOT DEFINED SOKOL_TOOLS_BIN_DIR)
  set(SOKOL_TOOLS_BIN_DIR "${CMAKE_SOURCE_DIR}/vendor/sokol-tools-bin")
endif()

if(CMAKE_HOST_APPLE)
  if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(SOKOL_SHDC_SUBDIR "osx_arm64")
  else()
    set(SOKOL_SHDC_SUBDIR "osx")
  endif()
  set(SOKOL_SHDC_EXT "")
elseif(CMAKE_HOST_LINUX)
  if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(SOKOL_SHDC_SUBDIR "linux_arm64")
  else()
    set(SOKOL_SHDC_SUBDIR "linux")
  endif()
  set(SOKOL_SHDC_EXT "")
elseif(CMAKE_HOST_WIN32)
  set(SOKOL_SHDC_SUBDIR "win32")
  set(SOKOL_SHDC_EXT ".exe")
else()
  message(FATAL_ERROR "Host system not supported")
endif()

set(SOKOL_SHDC_PATH "${SOKOL_TOOLS_BIN_DIR}/bin/${SOKOL_SHDC_SUBDIR}/sokol-shdc${SOKOL_SHDC_EXT}")
if(NOT EXISTS ${SOKOL_SHDC_PATH})
  message(FATAL_ERROR "sokol-shdc binary not found in ${SOKOL_SHDC_PATH}")
endif()

function(compile_glsl TARGET SHADER_FILE)
    set(SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE})
    set(OUTPUT_PATH "${SHADER_PATH}.h")
    add_custom_command(
        COMMAND ${SOKOL_SHDC_PATH}
            --input ${SHADER_PATH}
            --output ${OUTPUT_PATH}
            --slang ${SOKOL_SHDC_SLANG}
        DEPENDS ${SHADER_PATH}
        OUTPUT ${OUTPUT_PATH}
        VERBATIM
    )
    set_source_files_properties(${OUTPUT_PATH} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${OUTPUT_PATH})
endfunction()

function(compile_glsl_with_defines TARGET SHADER_FILE OUTPUT_SUFFIX DEFINES)
    set(SHADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE})
    set(OUTPUT_PATH "${SHADER_PATH}${OUTPUT_SUFFIX}.h")
    add_custom_command(
        COMMAND ${SOKOL_SHDC_PATH}
            --input ${SHADER_PATH}
            --output ${OUTPUT_PATH}
            --slang ${SOKOL_SHDC_SLANG}
            --defines ${DEFINES}
        DEPENDS ${SHADER_PATH}
        OUTPUT ${OUTPUT_PATH}
        COMMENT "Compiling ${SHADER_PATH} to ${OUTPUT_PATH}"
        VERBATIM
    )
    set_source_files_properties(${OUTPUT_PATH} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${OUTPUT_PATH})
endfunction()

function(target_add_sokol TARGET SOKOL_DEBUG)
  target_include_directories(${TARGET} PRIVATE
    ${SOKOL_INCLUDE_DIR}
    ${THIS_SOKOL_DIR}
  )
  if(APPLE)
    target_link_libraries(${TARGET} PRIVATE
            "-framework Cocoa"
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
    )
  elseif(ANDROID)
    target_link_libraries(${TARGET} PRIVATE
            EGL
            GLESv3
            android
            log
    )
  elseif(EMSCRIPTEN)
    target_link_libraries(${TARGET} PRIVATE
            -sMAX_WEBGL_VERSION=2
    )
  elseif(LINUX)
    target_compile_options(${TARGET} PRIVATE -pthread)
    target_link_libraries(${TARGET} PRIVATE -pthread)
    target_link_libraries(${TARGET} PRIVATE
            dl
            m
            GL
            X11
            Xcursor
            Xi
    )
  else()
    message(FATAL_ERROR "Target system not supported")
  endif()
  if(APPLE)
    target_sources(${TARGET} PRIVATE "${THIS_SOKOL_DIR}/sokol_impl.mm")
  else()
    target_sources(${TARGET} PRIVATE "${THIS_SOKOL_DIR}/sokol_impl.cpp")
  endif()

  if(SOKOL_DEBUG)
    target_compile_definitions(${TARGET} PRIVATE SOKOL_DEBUG)
  endif()
endfunction()

# Adds minimal required files for building tests
function(target_add_sokol_for_tests TARGET)
  target_include_directories(${TARGET} PRIVATE
    ${SOKOL_INCLUDE_DIR}
  )
  target_sources(${TARGET} PRIVATE "${THIS_SOKOL_DIR}/sokol_for_tests.cpp")
endfunction()
