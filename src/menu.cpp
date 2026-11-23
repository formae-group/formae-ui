// menu.cpp - the menu shell, its navigation and close rules, and the input
// binding table with capture (formae/ui.h).
//
// The shell fills the display: the game's picture under a backdrop of the
// given opacity, a bar of tabs with icon buttons at its right and an accent
// rule under it, then the chosen tab's categories in a sidebar beside the
// chosen category's page (a tab without categories draws one wide page).
// The sidebar and the page are NavFlattened children, so the arrows and the
// D-pad cross between the bar, the sidebar and the page. A bar button with a
// question asks in a confirmation first.
//
// Close rules (Menu::event, before ImGui sees the event): the game's menu
// key or button always closes; Escape and B close unless a popup is open or
// an item is active, in which case ImGui gets them. During a capture every
// key, button, axis and click goes to the binding instead: Escape (or B for a
// keyboard cell) cancels, Delete or Backspace clears, a click cancels, and
// the menu stays disabled until Enter, Space and A are released, so the
// button just bound does not start another capture.
#include "formae/ui.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace formae::ui {

Menu::Menu(std::vector<Tab> tabs, std::vector<BarButton> buttons)
    : tabs_(std::move(tabs)), buttons_(std::move(buttons)), category_(tabs_.size(), 0), ask_(buttons_.size(), 0)
{
}

void Menu::select(int tab, int category)
{
    if (tab < 0 || tab >= (int)tabs_.size()) {
        return;
    }
    tab_ = tab;
    category_[tab] = category;
}

void Menu::ask(int button)
{
    for (size_t i = 0; i < ask_.size(); i++) {
        ask_[i] = (int)i == button;
    }
}

void Menu::focus_page()
{
    focus_page_ = true;
}

void Menu::begin_capture(const BindingTable *table, int row, int column)
{
    capture_table_ = table;
    capture_row_ = row;
    capture_column_ = column;
}

void Menu::end_capture()
{
    capture_row_ = -1;
    capture_column_ = 0;
}

void Menu::notice(const char *text)
{
    notice_ = text ? text : "";
    notice_until_ = ImGui::GetTime() + 4.0;
}

int Menu::draw(int opacity_percent)
{
    const Theme &t = current().theme();
    const Fonts &F = current().fonts();
    if (hold_ && !ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown) && !ImGui::IsKeyDown(ImGuiKey_Enter) &&
        !ImGui::IsKeyDown(ImGuiKey_KeypadEnter) && !ImGui::IsKeyDown(ImGuiKey_Space)) {
        hold_ = false;
    }
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float b = F.base;
    const float margin = std::floor(b * 1.1f), gap = std::floor(b * 0.5f), bar_h = std::floor(b * 2.0f);
    const float line = std::floor(b * 0.12f) < 1.0f ? 1.0f : std::floor(b * 0.12f);
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0, 0), ds, (t.backdrop & ~IM_COL32_A_MASK) | (ImU32)(opacity_percent * 255 / 100) << IM_COL32_A_SHIFT);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ds);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##menu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const bool frozen = capture_row_ >= 0 || hold_;
    if (frozen) {
        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
        ImGui::BeginDisabled();
    }

    int pressed = -1;
    ImGui::SetCursorScreenPos(ImVec2(margin, margin));
    ImGui::PushFont(F.strong);
    for (int i = 0; i < (int)tabs_.size(); i++) {
        if (i) {
            ImGui::SameLine(0, std::floor(b * 0.25f));
        }
        const float w = ImGui::CalcTextSize(tabs_[i].name).x + std::floor(b * 1.6f);
        if (pick_button(tabs_[i].name, tab_ == i, ImVec2(w, bar_h))) {
            tab_ = i;
        }
    }
    ImGui::PopFont();
    const float count = (float)buttons_.size();
    ImGui::SetCursorScreenPos(ImVec2(ds.x - margin - bar_h * count - gap * (count - 1.0f), margin));
    for (size_t i = 0; i < buttons_.size(); i++) {
        if (i) {
            ImGui::SameLine(0, gap);
        }
        if (!icon_button(buttons_[i].id, buttons_[i].icon, bar_h, buttons_[i].danger)) {
            continue;
        }
        if (buttons_[i].question) {
            ask_[i] = 1;
        } else {
            pressed = (int)i;
        }
    }
    const float rule_y = margin + bar_h + gap;
    dl->AddRectFilled(ImVec2(margin, rule_y), ImVec2(ds.x - margin, rule_y + line), t.accent);

    const float top = rule_y + line + gap * 1.5f, body_h = ds.y - top - margin;
    const Tab &tab = tabs_[tab_];
    const int categories = (int)tab.categories.size();
    int &chosen = category_[tab_];
    if (chosen < 0 || chosen >= categories) {
        chosen = 0;
    }
    ImGui::SetCursorScreenPos(ImVec2(margin, top));
    float page_x = margin, page_w = ds.x - 2.0f * margin;
    if (categories) {
        const float side_w = std::floor(b * 10.0f);
        ImGui::BeginChild("##categories", ImVec2(side_w, body_h), ImGuiChildFlags_NavFlattened);
        for (int c = 0; c < categories; c++) {
            if (pick_button(tab.categories[c].name, chosen == c, ImVec2(side_w, std::floor(b * 1.9f)))) {
                chosen = c;
            }
        }
        ImGui::EndChild();
        const float sep_x = margin + side_w + gap;
        dl->AddRectFilled(ImVec2(sep_x, top), ImVec2(sep_x + line, top + body_h), t.line);
        page_x = sep_x + line + gap;
        page_w = ds.x - margin - page_x;
        if (page_w > b * 36.0f) {
            page_w = std::floor(b * 36.0f);
        }
    } else if (page_w > b * 60.0f) {
        page_w = std::floor(b * 60.0f);
    }
    ImGui::SetCursorScreenPos(ImVec2(page_x, top));
    ImGui::PushID(tab_ * 16 + chosen);
    ImGui::BeginChild("##page", ImVec2(page_w, body_h),
                      ImGuiChildFlags_NavFlattened | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (focus_page_) {
        ImGui::SetKeyboardFocusHere();
        ImGui::SetNavCursorVisible(true);
        focus_page_ = false;
    }
    if (categories) {
        if (tab.categories[chosen].draw) {
            tab.categories[chosen].draw();
        }
    } else if (tab.draw) {
        tab.draw();
    }
    ImGui::EndChild();
    ImGui::PopID();

    if (frozen) {
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
    }
    for (size_t i = 0; i < buttons_.size(); i++) {
        if (!buttons_[i].question) {
            continue;
        }
        bool asked = ask_[i] != 0;
        if (confirm((std::string(buttons_[i].id) + "-confirm").c_str(), &asked, buttons_[i].question,
                    buttons_[i].confirm ? buttons_[i].confirm : buttons_[i].question)) {
            pressed = (int)i;
        }
        ask_[i] = asked;
    }
    ImGui::End();
    busy_ = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ||
            ImGui::IsAnyItemActive();
    return pressed;
}

EventResult Menu::event(const InputEvent &ev)
{
    if (capture_row_ >= 0) {
        const BindingTable *table = capture_table_;
        const bool pad = table && capture_column_ < (int)table->columns.size() &&
                         table->columns[capture_column_].device == Device::gamepad;
        auto assign = [&](const InputEvent *e) -> const char * {
            return table && table->assign ? table->assign(capture_row_, capture_column_, e) : nullptr;
        };
        bool done = false;
        if (ev.type == InputEvent::key_down && !ev.repeat) {
            if (ev.clear) {
                assign(nullptr);
            } else if (!ev.cancel && !pad) {
                notice(assign(&ev));
            }
            done = ev.cancel || ev.clear || !pad;
        } else if (ev.type == InputEvent::button_down) {
            if (pad) {
                notice(assign(&ev));
            }
            done = pad || ev.back;
        } else if (pad && ev.type == InputEvent::axis && (ev.value > 20000 || ev.value < -20000)) {
            notice(assign(&ev));
            done = true;
        } else if (ev.type == InputEvent::mouse_down) {
            done = true;
        }
        if (done) {
            capture_row_ = -1;
            hold_ = true;
        }
        return ev.type == InputEvent::other ? EventResult::pass : EventResult::swallow;
    }
    if (ev.type == InputEvent::key_down && !ev.repeat) {
        if (!ev.cancel && ev.menu) {
            return EventResult::close;
        }
        if (ev.cancel && !busy_) {
            return EventResult::close;
        }
    }
    if (ev.type == InputEvent::button_down) {
        if (ev.menu) {
            return EventResult::close;
        }
        if (ev.back && !busy_) {
            return EventResult::close;
        }
    }
    return EventResult::pass;
}

void binding_table(Menu &menu, const BindingTable &table)
{
    const Theme &t = current().theme();
    const int columns = (int)table.columns.size();
    if (ImGui::BeginTable("##bindings", 1 + columns, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthStretch, 0.8f);
        for (int c = 0; c < columns; c++) {
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, 1.0f);
        }
        ImGui::TableNextRow();
        ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
        for (int c = 0; c < columns; c++) {
            ImGui::TableSetColumnIndex(1 + c);
            ImGui::TextUnformatted(table.columns[c].title);
        }
        ImGui::PopStyleColor();
        for (int r = 0; r < table.rows; r++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(table.label ? table.label(r) : "");
            for (int c = 0; c < columns; c++) {
                ImGui::TableSetColumnIndex(1 + c);
                ImGui::PushID(r * columns + c);
                const bool capturing = menu.capture_table_ == &table && menu.capture_row_ == r &&
                                       menu.capture_column_ == c;
                char text[64] = "";
                bool bound = true;
                if (capturing) {
                    std::snprintf(text, sizeof text, "%s", table.columns[c].prompt);
                } else if (table.text) {
                    bound = table.text(r, c, text, sizeof text);
                }
                const bool unbound = !capturing && !bound;
                ImGui::PushStyleColor(ImGuiCol_Text, capturing ? t.on_accent : unbound ? t.text_off : t.text);
                if (capturing) {
                    ImGui::PushStyleColor(ImGuiCol_Button, t.accent);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, t.accent);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, t.accent);
                }
                if (ImGui::Button((std::string(text) + "###binding").c_str(), ImVec2(-FLT_MIN, 0)) &&
                    menu.capture_row_ < 0) {
                    menu.begin_capture(&table, r, c);
                    menu.notice_.clear();
                }
                ImGui::PopStyleColor(capturing ? 4 : 1);
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    if (table.restore_label && button(table.restore_label) && table.restore) {
        table.restore();
    }
    if (!menu.notice_.empty() && ImGui::GetTime() < menu.notice_until_) {
        ImGui::PushStyleColor(ImGuiCol_Text, t.error_text);
        ImGui::TextWrapped("%s", menu.notice_.c_str());
        ImGui::PopStyleColor();
    }
}

} // namespace formae::ui
