// context.cpp - the ImGui context, the theme applied to ImGui's style, the
// fonts and the base size every measure is taken from (formae/ui.h).
//
// The base size is a 42nd of the drawable's height, at least 15 pixels at
// 96 DPI (scaled by the display's), at most a 20th of the height. Fonts are
// the system UI font (Segoe UI and Segoe UI Semibold on Windows) at the base
// size, 0.82 of it and 1.9 of it, with ImGui's own font as the fallback; they
// and the style are built again whenever the base size changes.
#include "formae/ui.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace formae::ui {

namespace {

Context *active = nullptr;

bool file_exists(const std::string &path)
{
    FILE *f = std::fopen(path.c_str(), "rb");
    if (f) std::fclose(f);
    return f != nullptr;
}

Fonts load_fonts(float base, const FontFiles &files)
{
    Fonts f;
    f.base = base;
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder b;
        b.AddRanges(io.Fonts->GetGlyphRangesDefault());
        static const ImWchar extra[] = { 0x2013, 0x2014, 0x2026, 0x2026, 0x2212, 0x2212, 0x00D7, 0x00D7, 0 };
        b.AddRanges(extra);
        b.BuildRanges(&ranges);
    }
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.PixelSnapH = true;
    if (!files.regular.empty() && file_exists(files.regular)) {
        const char *regular = files.regular.c_str();
        const char *heavy = !files.semibold.empty() && file_exists(files.semibold) ? files.semibold.c_str() : regular;
        f.body = io.Fonts->AddFontFromFileTTF(regular, base, &cfg, ranges.Data);
        f.strong = io.Fonts->AddFontFromFileTTF(heavy, base, &cfg, ranges.Data);
        f.small = io.Fonts->AddFontFromFileTTF(regular, std::floor(base * 0.82f), &cfg, ranges.Data);
        f.title = io.Fonts->AddFontFromFileTTF(heavy, std::floor(base * 1.9f), &cfg, ranges.Data);
        f.system = f.body && f.strong && f.small && f.title;
    }
    if (!f.system) {
        io.Fonts->Clear();
        ImFontConfig d;
        d.SizePixels = std::floor(base);
        f.body = f.strong = io.Fonts->AddFontDefault(&d);
        d.SizePixels = std::floor(base * 0.82f);
        f.small = io.Fonts->AddFontDefault(&d);
        d.SizePixels = std::floor(base * 1.9f);
        f.title = io.Fonts->AddFontDefault(&d);
    }
    io.FontDefault = f.body;
    return f;
}

ImU32 with_alpha(ImU32 c, int alpha)
{
    return (c & ~IM_COL32_A_MASK) | (ImU32)alpha << IM_COL32_A_SHIFT;
}

void apply_style(float b, const Theme &t)
{
    ImGuiStyle &s = ImGui::GetStyle();
    s = ImGuiStyle();
    s.WindowPadding = ImVec2(std::floor(b * 0.9f), std::floor(b * 0.7f));
    s.FramePadding = ImVec2(std::floor(b * 0.55f), std::floor(b * 0.3f));
    s.ItemSpacing = ImVec2(std::floor(b * 0.5f), std::floor(b * 0.42f));
    s.ItemInnerSpacing = ImVec2(std::floor(b * 0.35f), std::floor(b * 0.3f));
    s.CellPadding = ImVec2(std::floor(b * 0.3f), std::floor(b * 0.2f));
    s.IndentSpacing = b;
    s.ScrollbarSize = std::floor(b * 0.6f);
    s.GrabMinSize = std::floor(b * 0.8f);
    s.WindowBorderSize = s.ChildBorderSize = s.FrameBorderSize = 0.0f;
    s.PopupBorderSize = 1.0f;
    s.WindowRounding = s.ChildRounding = s.PopupRounding = std::floor(b * 0.35f);
    s.FrameRounding = s.GrabRounding = std::floor(b * 0.22f);
    s.ScrollbarRounding = b;
    s.SeparatorTextBorderSize = std::floor(b * 0.09f) < 1.0f ? 1.0f : std::floor(b * 0.09f);
    s.SeparatorTextPadding = ImVec2(std::floor(b * 0.9f), std::floor(b * 0.3f));
    s.SeparatorTextAlign = ImVec2(0.0f, 0.5f);
    s.DisabledAlpha = 0.45f;
    ImVec4 *c = s.Colors;
    auto col = [](ImU32 u) { return ImGui::ColorConvertU32ToFloat4(u); };
    const ImU32 clear = IM_COL32(0, 0, 0, 0);
    c[ImGuiCol_Text] = col(t.text);
    c[ImGuiCol_TextDisabled] = col(t.text_off);
    c[ImGuiCol_WindowBg] = c[ImGuiCol_ChildBg] = col(clear);
    c[ImGuiCol_PopupBg] = col(t.popup);
    c[ImGuiCol_Border] = col(t.line);
    c[ImGuiCol_FrameBg] = col(t.frame);
    c[ImGuiCol_FrameBgHovered] = col(t.frame_hovered);
    c[ImGuiCol_FrameBgActive] = col(t.frame_active);
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = col(t.accent);
    c[ImGuiCol_SliderGrabActive] = col(t.accent_hi);
    c[ImGuiCol_Button] = col(t.button);
    c[ImGuiCol_ButtonHovered] = col(t.button_hovered);
    c[ImGuiCol_ButtonActive] = col(t.button_active);
    c[ImGuiCol_Header] = col(with_alpha(t.accent, t.header_alpha));
    c[ImGuiCol_HeaderHovered] = col(t.soft_hover);
    c[ImGuiCol_HeaderActive] = col(t.soft_active);
    c[ImGuiCol_Separator] = c[ImGuiCol_SeparatorHovered] = c[ImGuiCol_SeparatorActive] = col(t.line);
    c[ImGuiCol_ScrollbarBg] = col(clear);
    c[ImGuiCol_ScrollbarGrab] = col(t.scrollbar);
    c[ImGuiCol_ScrollbarGrabHovered] = col(t.scrollbar_hovered);
    c[ImGuiCol_ScrollbarGrabActive] = col(t.scrollbar_active);
    c[ImGuiCol_TableHeaderBg] = c[ImGuiCol_TableRowBg] = col(clear);
    c[ImGuiCol_TableRowBgAlt] = col(t.row_alt);
    c[ImGuiCol_TableBorderStrong] = c[ImGuiCol_TableBorderLight] = col(t.line);
    c[ImGuiCol_TextSelectedBg] = col(with_alpha(t.accent, t.selection_alpha));
    c[ImGuiCol_NavCursor] = col(t.accent);
    c[ImGuiCol_ModalWindowDimBg] = col(t.modal_dim);
}

}

FontFiles system_font_files()
{
    FontFiles f;
#ifdef _WIN32
    const char *windir = std::getenv("WINDIR");
    const std::string dir = std::string(windir ? windir : "C:\\Windows") + "\\Fonts\\";
    f.regular = dir + "segoeui.ttf";
    f.semibold = dir + "seguisb.ttf";
#elif defined(__APPLE__)
    f.regular = f.semibold = "/System/Library/Fonts/SFNS.ttf";
#else
    f.regular = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    f.semibold = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
#endif
    return f;
}

float base_size(int drawable_height, float dpi)
{
    if (dpi <= 0.0f) dpi = 96.0f;
    float b = drawable_height / 42.0f;
    if (b < 15.0f * dpi / 96.0f) b = 15.0f * dpi / 96.0f;
    if (b > drawable_height / 20.0f) b = drawable_height / 20.0f;
    return std::floor(b < 10.0f ? 10.0f : b);
}

Context::Context(const Theme &theme, FontFiles files) : theme_(theme), files_(std::move(files))
{
    IMGUI_CHECKVERSION();
    imgui_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_);
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    active = this;
}

Context::~Context()
{
    if (imgui_) ImGui::DestroyContext(imgui_);
    if (active == this) active = nullptr;
}

bool Context::fit(float base)
{
    if (base == fonts_.base) return false;
    fonts_ = load_fonts(base, files_);
    apply_style(base, theme_);
    return true;
}

void Context::make_current()
{
    ImGui::SetCurrentContext(imgui_);
    active = this;
}

void Context::set_gamepad_navigation(bool on)
{
    ImGuiIO &io = ImGui::GetIO();
    if (on) io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    else io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
}

Context &current()
{
    IM_ASSERT(active && "formae::ui: no Context");
    return *active;
}

std::string glyphs(const char *utf8)
{
    std::string s = utf8 ? utf8 : "";
    if (!active || active->fonts().system) return s;
    static const char *const swaps[][2] = {
        { "\xE2\x88\x92", "-" }, { "\xC3\x97", "x" }, { "\xE2\x80\xA6", "..." }, { "\xE2\x80\x93", "-" }, { "\xE2\x80\x94", "-" },
    };
    for (const auto &w : swaps) {
        const std::string from = w[0], to = w[1];
        for (size_t at = s.find(from); at != std::string::npos; at = s.find(from, at + to.size())) s.replace(at, from.size(), to);
    }
    return s;
}

}
