// Sokol implementation file.

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#define SOKOL_GLES3
#elif defined(__linux__)
#define SOKOL_GLCORE
#else
#error "This file should be built for Android/Linux"
#endif

#include <imgui.h>

#define SOKOL_IMPL
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>
#include <sokol_time.h>
#include <util/sokol_debugtext.h>
#include <util/sokol_imgui.h>
#include <util/sokol_shape.h>
