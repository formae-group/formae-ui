// list.cpp - the reorderable list (formae/ui.h): rows of mods or packs in
// priority order, each with up and down buttons drawn with ImDrawList, an
// enable checkbox, a name, dimmed details and an optional error line.
//
// Every ImGui id of a row derives from ListRow::id, so the navigation focus,
// a pick-up and a pending scroll follow the row when it moves: pressing the
// focused up or down button again moves the same row again. Ctrl or Alt with
// Up or Down moves the focused row; the list owns the arrow keys while the
// modifier is held, so the focus does not move as well. Activating a row's
// name with A, Enter or Space picks the row up: a separate active id that
// claims the navigation directions, so the arrows, the D-pad and the left
// stick move the row, and A, Enter or Space put it down. B and Escape reach
// ImGui (Menu::busy() is true while an item is active), whose cancel clears
// the active id. A mouse drag on the name moves the row one place per frame
// while the pointer is past the place the row would take.
//
// A scrolling list is a NavFlattened child. ImGui keeps a single candidate
// for a directional move outside the window holding the focus, so a sidebar
// item scored earlier could shadow the rows and send Down from the page past
// the list. While the focus is in the page around the list, the rows are
// scored on their own and ImGui then weighs the best row against the page's
// own candidate by distance.
#include "formae/ui.h"
#include "imgui_internal.h"

#include <cmath>
#include <utility>

namespace formae::ui {

namespace {

void arrow(ImVec2 a, ImVec2 q, bool up)
{
    const ImVec2 c(std::floor((a.x + q.x) * 0.5f), std::floor((a.y + q.y) * 0.5f));
    const float r = (q.y - a.y) * 0.24f, d = up ? -1.0f : 1.0f;
    ImGui::GetWindowDrawList()->AddTriangleFilled(ImVec2(c.x - r, c.y - d * r * 0.45f), ImVec2(c.x + r, c.y - d * r * 0.45f),
                                                  ImVec2(c.x, c.y + d * r * 0.6f), ImGui::GetColorU32(ImGuiCol_Text));
}

}

ListEdit reorder_list(const char *id, std::vector<ListRow> &rows, const ListOptions &options)
{
    const Theme &t = current().theme();
    const Fonts &F = current().fonts();
    ImGuiContext &g = *ImGui::GetCurrentContext();
    const ImGuiStyle &s = ImGui::GetStyle();
    const int count = (int)rows.size();
    ListEdit edit;
    ImGui::PushID(id);
    ImGui::PushFont(F.body);
    ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
    if (options.header) ImGui::TextUnformatted(options.header);
    if (!count && options.empty) ImGui::TextUnformatted(options.empty);
    ImGui::PopStyleColor();
    if (!count) {
        ImGui::PopFont();
        ImGui::PopID();
        return edit;
    }

    ImGuiStorage *storage = ImGui::GetStateStorage();
    const ImGuiID scroll_key = ImGui::GetID("##scroll");
    const ImGuiID scroll_to = (ImGuiID)storage->GetInt(scroll_key, 0);
    const float b = ImGui::GetFrameHeight(), line_h = ImGui::GetTextLineHeight();
    const float gap = std::floor(F.base * 0.5f), half = std::floor(s.ItemSpacing.y * 0.5f);
    const float edge = std::floor(F.base * 0.1f) < 1.0f ? 1.0f : std::floor(F.base * 0.1f);
    const bool scrolling = options.visible_rows > 0.0f;
    const bool entering = scrolling && g.NavMoveScoringItems && g.NavWindow == ImGui::GetCurrentWindow();
    const ImGuiNavItemData other = g.NavMoveResultOther;
    if (entering) g.NavMoveResultOther.Clear();
    if (scrolling)
        ImGui::BeginChild("##rows", ImVec2(0.0f, std::floor(options.visible_rows * (b + s.ItemSpacing.y) - s.ItemSpacing.y)),
                          ImGuiChildFlags_NavFlattened);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float x0 = ImGui::GetCursorScreenPos().x, w = ImGui::GetContentRegionAvail().x;
    const bool modifier = g.IO.KeyCtrl || g.IO.KeyAlt;
    auto pitch = [&](int i) { return b + (rows[i].error.empty() ? 0.0f : line_h) + s.ItemSpacing.y; };
    ImGuiID moved = 0;
    for (int i = 0; i < count; i++) {
        ListRow &row = rows[i];
        ImGui::PushID(row.id.c_str());
        const ImGuiID up_id = ImGui::GetID("##up"), down_id = ImGui::GetID("##down"), on_id = ImGui::GetID("##on");
        const ImGuiID name_id = ImGui::GetID("##name"), grab_id = ImGui::GetID("##grab");
        const bool error = !row.error.empty(), grabbed = g.ActiveId == grab_id;
        const float h = b + (error ? line_h : 0.0f);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImRect band(x0, p.y - half, x0 + w, p.y + h + s.ItemSpacing.y - half);
        if (i % 2) dl->AddRectFilled(band.Min, band.Max, ImGui::GetColorU32(t.row_alt));

        int to = -1;
        ImGui::BeginDisabled(i == 0);
        if (ImGui::Button("##up", ImVec2(b, b))) to = i - 1;
        arrow(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        ImGui::EndDisabled();
        ImGui::SameLine(0.0f, s.ItemInnerSpacing.x);
        ImGui::BeginDisabled(i == count - 1);
        if (ImGui::Button("##down", ImVec2(b, b))) to = i + 1;
        arrow(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
        ImGui::EndDisabled();
        if (options.checkboxes) {
            ImGui::SameLine();
            bool on = row.enabled && !error;
            ImGui::BeginDisabled(error);
            if (ImGui::Checkbox("##on", &on) && edit.kind == ListEdit::none) {
                row.enabled = on;
                edit.kind = ListEdit::toggled;
                edit.row = i;
            }
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        const ImVec2 at = ImGui::GetCursorScreenPos();
        const bool pressed = ImGui::Selectable("##name", grabbed, ImGuiSelectableFlags_NoAutoClosePopups,
                                               ImVec2(ImGui::GetContentRegionAvail().x, h));
        const ImRect area(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        if (pressed && !grabbed && g.NavActivatePressedId == name_id) {
            ImGui::SetActiveID(grab_id, window);
            g.ActiveIdSource = g.NavInputSource;
        }
        if (g.ActiveId == grab_id) {
            ImGui::KeepAliveID(grab_id);
            g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_COUNT) - 1;
        }
        if (grabbed) {
            if (ImGui::IsKeyPressed(ImGuiKey_NavGamepadActivate, false) || ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) || ImGui::IsKeyPressed(ImGuiKey_Space, false))
                ImGui::ClearActiveID();
            else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp) ||
                     ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp))
                to = i - 1;
            else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown) ||
                     ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown))
                to = i + 1;
        } else if (modifier && (g.NavId == up_id || g.NavId == down_id || g.NavId == on_id || g.NavId == name_id)) {
            if (ImGui::GetKeyOwner(ImGuiKey_UpArrow) == name_id && ImGui::IsKeyPressed(ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, name_id))
                to = i - 1;
            else if (ImGui::GetKeyOwner(ImGuiKey_DownArrow) == name_id &&
                     ImGui::IsKeyPressed(ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, name_id))
                to = i + 1;
            ImGui::SetKeyOwner(ImGuiKey_UpArrow, name_id);
            ImGui::SetKeyOwner(ImGuiKey_DownArrow, name_id);
        }
        if (options.drag && ImGui::IsItemActive() && g.ActiveIdSource == ImGuiInputSource_Mouse) {
            const float y = g.IO.MousePos.y;
            if (i > 0 && y < area.Min.y + ImMin(0.0f, area.GetHeight() - pitch(i - 1))) to = i - 1;
            else if (i < count - 1 && y > area.Min.y + ImMax(area.GetHeight(), pitch(i + 1))) to = i + 1;
        }
        if (to >= 0 && to < count && edit.kind == ListEdit::none) {
            edit.kind = ListEdit::moved;
            edit.row = i;
            edit.to = to;
            moved = name_id;
        }

        const ImVec2 text(std::floor(at.x), std::floor(at.y + s.FramePadding.y));
        dl->PushClipRect(area.Min, area.Max, true);
        dl->AddText(text, ImGui::GetColorU32(t.text), row.name.c_str());
        if (!row.detail.empty())
            dl->AddText(ImVec2(text.x + std::floor(ImGui::CalcTextSize(row.name.c_str()).x) + gap, text.y), ImGui::GetColorU32(t.text_dim),
                        row.detail.c_str());
        if (error)
            dl->AddText(ImVec2(text.x, std::floor(at.y + b - s.FramePadding.y * 0.5f)), ImGui::GetColorU32(t.error_text), row.error.c_str());
        dl->PopClipRect();
        if (g.ActiveId == grab_id) dl->AddRect(band.Min, band.Max, ImGui::GetColorU32(t.accent), s.FrameRounding, 0, edge);
        if (scroll_to == name_id) {
            ImGui::ScrollToRect(window, band, ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_KeepVisibleEdgeY);
            storage->SetInt(scroll_key, 0);
        }
        ImGui::PopID();
    }
    if (scrolling) ImGui::EndChild();
    if (entering && g.NavMoveResultOther.ID == 0) g.NavMoveResultOther = other;
    if (edit.kind == ListEdit::moved) {
        std::swap(rows[edit.row], rows[edit.to]);
        storage->SetInt(scroll_key, (int)moved);
    }
    ImGui::PopFont();
    ImGui::PopID();
    return edit;
}

}
