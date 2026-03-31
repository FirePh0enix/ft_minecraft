#pragma once

#include <imgui.h>

static inline void imguitk_centered_next_widget(const char *label, float alignment = 0.5f)
{
    ImGuiStyle& style = ImGui::GetStyle();

    const float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
    const float avail = ImGui::GetContentRegionAvail().x;

    const float off = (avail - size) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}
