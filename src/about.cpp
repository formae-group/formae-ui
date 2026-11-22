// about.cpp - the About page and the developer logo (formae/ui.h).
//
// The page shows the game's title, its version and build date, the
// developer's name with its logo, a credits table (name, author, licence)
// and a disclaimer, all passed in by the game. On a page at least 40 base
// sizes wide the identity and the credits sit side by side; narrower, one
// above the other.
//
// The logo is a PNG (embedded with formae_ui_embed_file) decoded by
// stb_image. Its coverage is drawn in its own colour, area-averaged from the
// original to the height asked for; the game's TextureCallbacks turn the
// straight-alpha 0xAARRGGBB pixels into a linearly filtered texture of its
// renderer.
#include "formae/ui.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "third_party/stb_image.h"

#include <cmath>

namespace formae::ui {

Logo::Logo(const unsigned char *png, size_t size, TextureCallbacks textures) : textures_(textures)
{
    int n = 0;
    unsigned char *px = png ? stbi_load_from_memory(png, (int)size, &sw_, &sh_, &n, 4) : nullptr;
    if (!px) {
        sw_ = sh_ = 0;
        return;
    }
    alpha_.resize((size_t)sw_ * (size_t)sh_);
    for (size_t i = 0; i < alpha_.size(); i++) {
        alpha_[i] = px[i * 4 + 3];
        if (alpha_[i] == 255) rgb_ = (uint32_t)px[i * 4] << 16 | (uint32_t)px[i * 4 + 1] << 8 | px[i * 4 + 2];
    }
    stbi_image_free(px);
}

Logo::~Logo()
{
    release();
}

ImTextureID Logo::at(int height)
{
    if (!sw_ || height <= 0 || !textures_.create) return 0;
    if (height > sh_) height = sh_;
    if (tex_ && height == h_) return tex_;
    release();
    h_ = height;
    w_ = (int)std::lround((double)sw_ * h_ / sh_);
    std::vector<uint32_t> out((size_t)w_ * (size_t)h_);
    for (int y = 0; y < h_; y++) {
        const int y0 = y * sh_ / h_, y1 = (y + 1) * sh_ / h_ > y0 ? (y + 1) * sh_ / h_ : y0 + 1;
        for (int x = 0; x < w_; x++) {
            const int x0 = x * sw_ / w_, x1 = (x + 1) * sw_ / w_ > x0 ? (x + 1) * sw_ / w_ : x0 + 1;
            unsigned sum = 0;
            for (int sy = y0; sy < y1; sy++)
                for (int sx = x0; sx < x1; sx++) sum += alpha_[(size_t)sy * sw_ + sx];
            const unsigned a = sum / (unsigned)((y1 - y0) * (x1 - x0));
            out[(size_t)y * w_ + x] = a << 24 | rgb_;
        }
    }
    tex_ = textures_.create(textures_.user, out.data(), w_, h_);
    return tex_;
}

void Logo::release()
{
    if (tex_ && textures_.destroy) textures_.destroy(textures_.user, tex_);
    tex_ = 0;
}

namespace {

void identity(const About &a, const Fonts &F, const Theme &t)
{
    ImGui::PushFont(F.title);
    ImGui::TextUnformatted(a.title ? a.title : "");
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
    if (a.version) ImGui::Text("%s %s", a.version_label, a.version);
    if (a.build_date) ImGui::Text("%s %s", a.built_label, a.build_date);
    ImGui::PopStyleColor();
    if (!a.developer && !a.logo) return;
    ImGui::Dummy(ImVec2(0, std::floor(F.base * 0.6f)));
    if (a.developer) {
        ImGui::PushFont(F.strong);
        ImGui::TextUnformatted(a.developer);
        ImGui::PopFont();
    }
    ImGui::Dummy(ImVec2(0, std::floor(F.base * 0.3f)));
    if (!a.logo) return;
    if (ImTextureID tex = a.logo->at((int)std::floor(F.base * 6.5f)))
        ImGui::Image(tex, ImVec2((float)a.logo->width(), (float)a.logo->height()));
}

void credits(const About &a, const Fonts &F, const Theme &t)
{
    section(a.credits_title);
    if (ImGui::BeginTable("##credits", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("##by", ImGuiTableColumnFlags_WidthStretch, 1.3f);
        ImGui::TableSetupColumn("##licence", ImGuiTableColumnFlags_WidthStretch, 1.1f);
        for (const Credit &row : a.credits) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushFont(F.strong);
            ImGui::TextWrapped("%s", row.name);
            ImGui::PopFont();
            ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped("%s", row.by);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextWrapped("%s", row.licence);
            ImGui::PopStyleColor();
        }
        ImGui::EndTable();
    }
    if (!a.disclaimer) return;
    ImGui::Dummy(ImVec2(0, std::floor(F.base * 0.5f)));
    ImGui::PushStyleColor(ImGuiCol_Text, t.text_dim);
    ImGui::TextWrapped("%s", a.disclaimer);
    ImGui::PopStyleColor();
}

}

void about_page(const About &about)
{
    const Fonts &F = current().fonts();
    const Theme &t = current().theme();
    if (ImGui::GetContentRegionAvail().x < F.base * 40.0f) {
        identity(about, F, t);
        ImGui::Dummy(ImVec2(0, F.base));
        credits(about, F, t);
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(std::floor(F.base * 1.2f), 0));
    const bool table = ImGui::BeginTable("##about", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV);
    ImGui::PopStyleVar();
    if (!table) return;
    ImGui::TableSetupColumn("##identity", ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableSetupColumn("##credits", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    identity(about, F, t);
    ImGui::TableSetColumnIndex(1);
    credits(about, F, t);
    ImGui::EndTable();
}

}
