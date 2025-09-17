#include "wad_display.h"

#include <imgui.h>
#include <sokol_app.h>
#include <util/sokol_imgui.h>

#include "common/slog.h"
#include "wad3.h"


void WAD3Display::add_wad(const WAD3Parser& wad) {
    WADEntry wad_entry;
    wad_entry.name = wad.name;
    for (const WAD3Miptex& miptex : wad.miptexs) {
        sg_image_desc img_desc = {};
        img_desc.width = int(miptex.width);
        img_desc.height = int(miptex.height);
        img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        img_desc.data.mip_levels[0].ptr = miptex.mipmaps[0].data.data();
        img_desc.data.mip_levels[0].size = miptex.mipmaps[0].data.size();
        sg_image image = sg_make_image(img_desc);
        sg_view_desc view_desc = {};
        view_desc.texture.image = image;
        sg_view image_view = sg_make_view(view_desc);

        TextureEntry entry;
        entry.name = miptex.name;
        entry.image = image;
        entry.image_view = image_view;
        entry.width = miptex.width;
        entry.height = miptex.height;
        wad_entry.textures.push_back(std::move(entry));
    }
    wads.push_back(std::move(wad_entry));
}

void WAD3Display::render() {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("WAD Texture Browser")) {
        if (loading) {
            ImGui::Text("Loading (%zu complete)...", wads.size());
            ImGui::End();
            return;
        }

        if (ImGui::BeginTable("WAD BrowserTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("WAD Tree View", ImGuiTableColumnFlags_WidthFixed, 300.0f);
            ImGui::TableSetupColumn("WAD Image Preview", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            char filter_buffer[1000];
            strncpy(filter_buffer, filter_text.c_str(), sizeof(filter_buffer) - 1);
            filter_buffer[sizeof(filter_buffer) - 1] = '\0';
            if (ImGui::InputText("Filter", filter_buffer, sizeof(filter_buffer))) {
                filter_text = filter_buffer;
            }
            if (!filter_text.empty()) {
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    filter_text.clear();
                }
            }

            ImGui::BeginChild("WADTreeViewWindow", ImVec2(0, 0), ImGuiChildFlags_Borders);
            for (int wad_idx = 0; wad_idx < int(wads.size()); wad_idx++) {
                const WADEntry& wad = wads[wad_idx];

                if (ImGui::TreeNodeEx(wad.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth)) {
                    for (int texture_idx = 0; texture_idx < int(wad.textures.size()); texture_idx++) {
                        const TextureEntry& texture = wad.textures[texture_idx];
                        bool matches = true;
                        if (!filter_text.empty()) {
                            matches = texture.name.find(filter_text) != std::string::npos;
                        }

                        if (matches) {
                            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                       ImGuiTreeNodeFlags_SpanFullWidth;
                            if (selected_wad_index == wad_idx && selected_texture_index == texture_idx) {
                                flags |= ImGuiTreeNodeFlags_Selected;
                            }

                            if (ImGui::TreeNodeEx(texture.name.c_str(), flags)) {
                                if (ImGui::IsItemClicked()) {
                                    selected_wad_index = wad_idx;
                                    selected_texture_index = texture_idx;
                                }
                            }

                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("Size: %ux%u", texture.width, texture.height);
                                ImGui::EndTooltip();
                            }
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);

            if (ImGui::BeginChild("WADImagePreviewWindow")) {
                if (selected_wad_index >= 0 && selected_texture_index >= 0) {
                    const TextureEntry& selected_texture = wads[selected_wad_index].textures[selected_texture_index];
                    ImGui::Text("Texture: %s, size: %ux%u", selected_texture.name.c_str(), selected_texture.width,
                                selected_texture.height);
                    ImGui::Checkbox("Scale", &scale_image);
                    ImGui::Separator();

                    ImVec2 image_size = ImVec2(float(selected_texture.width), float(selected_texture.height));
                    if (scale_image) {
                        ImVec2 avail_region = ImGui::GetContentRegionAvail();
                        float min_scale = std::min(avail_region.x / image_size.x, avail_region.y / image_size.y);
                        image_size.x *= min_scale;
                        image_size.y *= min_scale;
                    }

                    ImTextureID tex_id = ImTextureID(simgui_imtextureid(selected_texture.image_view));
                    ImGui::Image(tex_id, image_size);
                }
            }
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void WAD3Display::destroy() {
    for (WADEntry& entry : wads) {
        entry.destroy();
    }
}

void WAD3Display::WADEntry::destroy() {
    for (TextureEntry& entry : textures) {
        entry.destroy();
    }
}

void WAD3Display::TextureEntry::destroy() {
    sg_destroy_view(image_view);
    image_view = {0};
    sg_destroy_image(image);
    image = {0};
}
