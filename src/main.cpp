#include <imgui.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>
#include <sokol_time.h>
#include <util/sokol_debugtext.h>
#include <util/sokol_imgui.h>

#include "common/defer.h"
#include "common/io.h"
#include "common/slog.h"
#include "common/sync.h"
#include "common/thread.h"
#include "hl1/wad3.h"
#include "hl1/wad_display.h"


const char* wads_list_path = "data/hl1/wads.txt";

struct DisplayState {
    WAD3Display wad_display;

    MPMCQueue<WAD3Parser> parsed_wads;
    TaskLatch parsed_wads_latch;
    bool finished_loading = false;
};

DisplayState g_state;

bool start_parsing() {
    std::vector<std::string> wad_file_list;
    if (!file_read_lines(wads_list_path, wad_file_list)) {
        return false;
    }
    g_state.parsed_wads_latch.reset(wad_file_list.size());

    std::string wad_dir = path_get_directory(wads_list_path);
    for (const std::string& wad_file : wad_file_list) {
        std::string wad_path = path_join(wad_dir.c_str(), wad_file.c_str());
        thread_pool().submit([wad_path]() mutable {
            DEFER(g_state.parsed_wads_latch.count_down());
            FileContents wad_contents;
            if (file_read_contents(wad_path.c_str(), wad_contents)) {
                WAD3Parser wad;
                wad.parse(wad_contents);
                g_state.parsed_wads.push(std::move(wad));
            }
        });
    }
    return true;
}

void process_parsed() {
    if (g_state.finished_loading) {
        return;
    }
    WAD3Parser wad;
    while (g_state.parsed_wads.try_pop(wad)) {
        g_state.wad_display.add_wad(wad);
    }
    if (g_state.parsed_wads_latch.done()) {
        g_state.wad_display.loading = false;
        g_state.finished_loading = true;
    }
}

void init_cb() {
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    desc.image_pool_size = 10000;  // Increased to accommodate many WAD textures
    sg_setup(desc);

    sdtx_desc_t sdtx_desc = {};
    sdtx_desc.logger.func = slog_func;
    sdtx_desc.fonts[0] = sdtx_font_cpc();
    sdtx_setup(sdtx_desc);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetStyle().FrameBorderSize = 1.0f;
    ImGui::GetStyle().FrameRounding = 3.0f;
    ImGui::GetStyle().WindowRounding = 5.0f;
    simgui_desc_t simgui_desc = {};
    simgui_desc.logger.func = slog_func;
    simgui_setup(simgui_desc);

    if (!start_parsing()) {
#if !defined(__EMSCRIPTEN__)
        return sapp_quit();
#endif
    }
}

void frame_cb() {
    process_parsed();

    simgui_frame_desc_t simgui_frame_desc = {};
    simgui_frame_desc.width = sapp_width();
    simgui_frame_desc.height = sapp_height();
    simgui_frame_desc.delta_time = sapp_frame_duration();
    simgui_frame_desc.dpi_scale = sapp_dpi_scale();
    simgui_new_frame(simgui_frame_desc);

    sg_pass pass = {};
    pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass.action.colors[0].clear_value = {0.0, 0.0, 0.0, 1.0};
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(pass);

    g_state.wad_display.render();

    simgui_render();
    sg_end_pass();
    sg_commit();
}

void cleanup_cb() {
    simgui_shutdown();
    sg_shutdown();
}

void event_cb(const sapp_event* event) {
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN && event->key_code == SAPP_KEYCODE_Q &&
        ((event->modifiers & (SAPP_MODIFIER_CTRL | SAPP_MODIFIER_SUPER)) != 0)) {
#if !defined(__EMSCRIPTEN__)
        sapp_request_quit();
#endif
    }

    if (simgui_handle_event(event)) {
        return;
    }
}

sapp_desc sokol_main(int argc, char** argv) {
    stm_setup();

    sapp_desc desc = {};
    desc.init_cb = init_cb;
    desc.frame_cb = frame_cb;
    desc.cleanup_cb = cleanup_cb;
    desc.event_cb = event_cb;
    desc.width = 1200;
    desc.height = 1000;
    desc.window_title = "Sokol Experiment";
    desc.logger.func = slog_func;
    return desc;
}
