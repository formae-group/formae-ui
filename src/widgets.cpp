// widgets.cpp - the controls pages are made of (formae/ui.h): titled
// section separators, the accent "Restart to apply" tag, toggles, combos,
// sliders with - and + buttons that repeat while held, full-width buttons,
// the tab and category buttons, the bar's icon buttons drawn with ImDrawList
// and the centred confirmation dialog.
//
// A tab or category button is filled with the accent while chosen; with a
// keyboard or a controller, moving onto one chooses it (only a move: the
// focus that falls back onto one when the page under it changes does not).
// A confirmation closes on its Cancel button, Escape or the controller's B.
#include "formae/ui.h"
#include "imgui_internal.h"

#include <cmath>

namespace formae::ui {

namespace {

const float PI = 3.14159265f;

}

void section(const char *title)
{
    ImGui::PushStyleColor(ImGuiCol_Text, current().theme().text_dim);
    ImGui::SeparatorText(title);
    ImGui::PopStyleColor();
}

void tag(const char *text)
{
    if (!text) return;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, current().theme().accent);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

bool toggle(const char *label, bool *value)
{
    return ImGui::Checkbox(label, value);
}

bool combo(const char *label, int *value, const char *const *names, int count, const char *note)
{
    bool changed = false;
    ImGui::PushID(label);
    const float x0 = ImGui::GetCursorPosX(), full = ImGui::GetContentRegionAvail().x, w = std::floor(full * 0.5f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    tag(note);
    ImGui::SameLine(x0 + full - w);
    ImGui::SetNextItemWidth(w);
    const int cur = *value < 0 || *value >= count ? 0 : *value;
    if (ImGui::BeginCombo("##choice", names[cur])) {
        for (int i = 0; i < count; i++) {
            const bool selected = i == cur;
            if (ImGui::Selectable(names[i], selected) && !selected) {
                *value = i;
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}

bool slider(const char *label, int *value, int lo, int hi, int step, const char *format, const char *note)
{
    const int was = *value;
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    tag(note);
    const ImGuiStyle &s = ImGui::GetStyle();
    const float b = ImGui::GetFrameHeight(), w = ImGui::GetContentRegionAvail().x;
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    if (ImGui::Button(glyphs("\xE2\x88\x92##dec").c_str(), ImVec2(b, b))) *value -= step;
    ImGui::SameLine(0, s.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(w - 2.0f * (b + s.ItemInnerSpacing.x));
    ImGui::SliderInt("##value", value, lo, hi, glyphs(format).c_str(), ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput);
    ImGui::SameLine(0, s.ItemInnerSpacing.x);
    if (ImGui::Button("+##inc", ImVec2(b, b))) *value += step;
    ImGui::PopItemFlag();
    ImGui::PopID();
    if (*value < lo) *value = lo;
    if (*value > hi) *value = hi;
    return *value != was;
}

bool slider_percent(const char *label, int *value, int lo, int hi, int step, const char *note)
{
    return slider(label, value, lo, hi, step, "%d%%", note);
}

bool button(const char *label)
{
    return ImGui::Button(label, ImVec2(-FLT_MIN, 0));
}

bool pick_button(const char *label, bool chosen, ImVec2 size)
{
    const Theme &t = current().theme();
    ImGui::PushStyleColor(ImGuiCol_Button, chosen ? t.accent : IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chosen ? t.accent_hi : t.soft_hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, chosen ? t.accent_hi : t.soft_active);
    ImGui::PushStyleColor(ImGuiCol_Text, chosen ? t.on_accent : t.text);
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return pressed || (!chosen && ImGui::GetCurrentContext()->NavJustMovedToId == ImGui::GetItemID());
}

bool icon_button(const char *id, Icon icon, float size, bool danger)
{
    const Theme &t = current().theme();
    if (danger) {
        ImGui::PushStyleColor(ImGuiCol_Button, t.danger);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, t.danger_hi);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, t.danger_hi);
    }
    const bool pressed = ImGui::Button(id, ImVec2(size, size));
    if (danger) ImGui::PopStyleColor(3);
    const ImVec2 a = ImGui::GetItemRectMin(), q = ImGui::GetItemRectMax();
    const ImVec2 c((a.x + q.x) * 0.5f, (a.y + q.y) * 0.5f);
    const float r = size * 0.24f, th = size * 0.075f < 1.5f ? 1.5f : size * 0.075f;
    const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    switch (icon) {
    case Icon::quit:
        dl->PathArcTo(c, r, -PI * 0.5f + 0.75f, PI * 1.5f - 0.75f, 28);
        dl->PathStroke(col, 0, th);
        dl->AddLine(ImVec2(c.x, c.y - r * 1.3f), ImVec2(c.x, c.y - r * 0.2f), col, th);
        break;
    case Icon::reset: {
        const float a0 = -PI * 0.5f, a1 = a0 + PI * 1.5f;
        dl->PathArcTo(c, r, a0, a1, 28);
        dl->PathStroke(col, 0, th);
        const ImVec2 n(std::cos(a1), std::sin(a1)), d(-n.y, n.x), p(c.x + n.x * r, c.y + n.y * r);
        const float h = r * 0.62f;
        dl->AddTriangleFilled(ImVec2(p.x + d.x * h, p.y + d.y * h), ImVec2(p.x + n.x * h - d.x * h * 0.25f, p.y + n.y * h - d.y * h * 0.25f),
                              ImVec2(p.x - n.x * h - d.x * h * 0.25f, p.y - n.y * h - d.y * h * 0.25f), col);
        break;
    }
    case Icon::resume:
        dl->AddTriangleFilled(ImVec2(c.x - r * 0.7f, c.y - r * 0.95f), ImVec2(c.x - r * 0.7f, c.y + r * 0.95f),
                              ImVec2(c.x + r * 0.95f, c.y), col);
        break;
    }
    return pressed;
}

bool confirm(const char *id, bool *ask, const char *question, const char *yes, const char *cancel)
{
    const Fonts &F = current().fonts();
    if (*ask) {
        ImGui::OpenPopup(id);
        *ask = false;
    }
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f, ds.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove))
        return false;
    bool confirmed = false;
    ImGui::PushFont(F.strong);
    ImGui::TextUnformatted(question);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, std::floor(F.base * 0.4f)));
    const float w = std::floor(F.base * 7.0f);
    if (ImGui::Button(yes, ImVec2(w, 0))) {
        confirmed = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button(cancel, ImVec2(w, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false) ||
        ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
    return confirmed;
}

}
