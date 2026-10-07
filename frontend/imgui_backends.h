#ifndef IMGUI_BACKENDS_H
#define IMGUI_BACKENDS_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool gb_imgui_init(SDL_Window* window, SDL_Renderer* renderer);
void gb_imgui_new_frame(void);
bool gb_imgui_process_event(const SDL_Event* event);
void gb_imgui_render(SDL_Renderer* renderer);
void gb_imgui_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
