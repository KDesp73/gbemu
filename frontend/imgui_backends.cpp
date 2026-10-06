#include "imgui_backends.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

extern "C"
{

bool gb_imgui_init(SDL_Window* window, SDL_Renderer* renderer)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer))
        return false;
    if (!ImGui_ImplSDLRenderer3_Init(renderer))
        return false;
    return true;
}

void gb_imgui_new_frame(void)
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

bool gb_imgui_process_event(const SDL_Event* event)
{
    return ImGui_ImplSDL3_ProcessEvent(event);
}

void gb_imgui_render(SDL_Renderer* renderer)
{
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(renderer, 0.09f, 0.09f, 0.11f, 1.0f);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void gb_imgui_shutdown(void)
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

}
