// first_run.cpp - the first-run screen (formae/ui.h): the game's title in
// a bar over an accent rule, and below it a centred column with a title, an
// explanation, the primary action (accent) and the secondary one, a status
// line (success, or an error with a smaller detail) and a footer at the
// bottom. The game supplies every text and acts on what draw returns;
// Escape and B count as the secondary action. Buttons are disabled while the
// game reports itself busy.
#include "formae/ui.h"

#include <cmath>

namespace formae::ui {

FirstRunAction FirstRunScreen::draw(const FirstRunContent &content)
{
    const Fonts &F = current().fonts();
    const Theme &t = current().theme();
    FirstRunAction action = FirstRunAction::none;
    const float b = F.base;
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float margin = std::floor(b * 1.1f), gap = std::floor(b * 0.5f), bar_h = std::floor(b * 2.0f);
    const float line = std::floor(b * 0.12f) < 1.0f ? 1.0f : std::floor(b * 0.12f);
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ds, t.backdrop);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ds);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##first-run", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    if (content.app_title)
        dl->AddText(F.strong, F.strong->FontSize, ImVec2(margin, margin + std::floor((bar_h - F.strong->FontSize) * 0.5f)), t.text,
                    content.app_title);
    dl->AddRectFilled(ImVec2(margin, margin + bar_h + gap), ImVec2(ds.x - margin, margin + bar_h + gap + line), t.accent);

    float col_w = ds.x - margin * 2.0f;
    if (col_w > b * 30.0f) col_w = std::floor(b * 30.0f);
    const float x = std::floor((ds.x - col_w) * 0.5f);
    ImGui::SetCursorScreenPos(ImVec2(x, std::floor(margin + bar_h + gap + (ds.y - margin - bar_h) * 0.14f)));
    ImGui::PushTextWrapPos(x + col_w);
    ImGui::PushFont(F.title);
    ImGui::TextUnformatted(content.title ? content.title : "");
    ImGui::PopFont();
    ImGui::SetCursorPosX(x);
    ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
    ImGui::TextWrapped("%s", content.text ? content.text : "");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, b));
    ImGui::SetCursorPosX(x);
    ImGui::BeginDisabled(content.busy);
    const float button_h = std::floor(b * 2.2f);
    ImGui::PushFont(F.strong);
    ImGui::PushStyleColor(ImGuiCol_Button, t.accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, t.accent_hi);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, t.accent_hi);
    ImGui::PushStyleColor(ImGuiCol_Text, t.on_accent);
    if (ImGui::Button(glyphs(content.primary).c_str(), ImVec2(std::floor(b * 11.0f), button_h))) action = FirstRunAction::primary;
    ImGui::PopStyleColor(4);
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine(0, gap);
    if (ImGui::Button(glyphs(content.secondary).c_str(), ImVec2(std::floor(b * 6.0f), button_h))) action = FirstRunAction::secondary;
    ImGui::PopFont();
    ImGui::EndDisabled();
    if (first_frame_) ImGui::SetNavCursorVisible(true);
    first_frame_ = false;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false))
        action = FirstRunAction::secondary;
    ImGui::Dummy(ImVec2(0, gap));
    ImGui::SetCursorPosX(x);
    if (content.status == Status::ok && content.status_text) {
        ImGui::PushStyleColor(ImGuiCol_Text, t.ok_text);
        ImGui::TextUnformatted(content.status_text);
        ImGui::PopStyleColor();
    } else if (content.status == Status::error && content.status_text) {
        ImGui::PushStyleColor(ImGuiCol_Text, t.error_text);
        ImGui::TextWrapped("%s", content.status_text);
        ImGui::PopStyleColor();
        if (content.detail && *content.detail) {
            ImGui::SetCursorPosX(x);
            ImGui::PushFont(F.small);
            ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
            ImGui::TextWrapped("%s", content.detail);
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
    }
    ImGui::PopTextWrapPos();
    if (content.footer)
        dl->AddText(F.small, F.small->FontSize, ImVec2(x, ds.y - margin - F.small->FontSize * 2.4f), t.text_off, content.footer,
                    nullptr, col_w);
    ImGui::End();
    return action;
}

}
