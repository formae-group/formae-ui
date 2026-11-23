// minimal.cpp - the README's minimal example: a two-page settings menu on
// SDL2 and Dear ImGui's SDL_Renderer backend. It is built with the other
// examples so the README's code always compiles. It opens a window.
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "formae/ui.h"

namespace ui = formae::ui;

static int volume = 80;
static bool vsync = true;

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    SDL_Window *window = SDL_CreateWindow("My Port", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                                          SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    {
        ui::Context context;
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        ui::Menu menu({ { "Settings",
                          { { "Audio",
                              [] {
                                  ui::section("Volume");
                                  ui::slider_percent("Master volume", &volume, 0, 100, 5);
                              } },
                            { "Video",
                              [] {
                                  ui::section("Display");
                                  ui::toggle("VSync", &vsync);
                              } } } } },
                      { { "##resume", ui::Icon::resume } });

        bool open = true;
        while (open) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ui::InputEvent input;
                if (event.type == SDL_KEYDOWN) {
                    input.type = ui::InputEvent::key_down;
                    input.cancel = event.key.keysym.scancode == SDL_SCANCODE_ESCAPE;
                }
                if (event.type == SDL_QUIT || menu.event(input) == ui::EventResult::close) {
                    open = false;
                } else {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                }
            }
            int width = 0, height = 0;
            SDL_GetRendererOutputSize(renderer, &width, &height);
            if (context.fit(ui::base_size(height, 96.0f))) {
                ImGui_ImplSDLRenderer2_DestroyFontsTexture();
            }

            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            if (menu.draw(80) == 0) {
                open = false;
            }
            ImGui::Render();
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        }
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
