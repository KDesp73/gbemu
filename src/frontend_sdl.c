#include "frontend.h"
#include <stdlib.h>
#include <SDL.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int scale;
} SDLPriv;

static bool sdl_init(Frontend* fe, int width, int height)
{
    SDLPriv* priv = fe->priv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "[ERR] SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    const char* scale_env = getenv("EMU_SCALE");
    priv->scale = scale_env ? atoi(scale_env) : 4;
    if (priv->scale < 1) priv->scale = 1;

    priv->window = SDL_CreateWindow(
        "EMU",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width * priv->scale, height * priv->scale,
        SDL_WINDOW_SHOWN
    );
    if (!priv->window) {
        fprintf(stderr, "[ERR] SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }

    priv->renderer = SDL_CreateRenderer(priv->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!priv->renderer) {
        fprintf(stderr, "[ERR] SDL_CreateRenderer: %s\n", SDL_GetError());
        return false;
    }

    priv->texture = SDL_CreateTexture(priv->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        width, height);
    if (!priv->texture) {
        fprintf(stderr, "[ERR] SDL_CreateTexture: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

static void sdl_render(Frontend* fe, const uint32_t* buffer,
                       int width, int height)
{
    SDLPriv* priv = fe->priv;

    SDL_UpdateTexture(priv->texture, NULL, buffer, width * sizeof(uint32_t));
    SDL_RenderCopy(priv->renderer, priv->texture, NULL, NULL);
    SDL_RenderPresent(priv->renderer);
}

static void sdl_poll_events(Frontend* fe, bool* running)
{
    (void)fe;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            *running = false;
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                *running = false;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            bool pressed = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.sym) {
            // Row 1: Down, Up, Left, Right
            case SDLK_DOWN:    (void)pressed; break;
            case SDLK_UP:      (void)pressed; break;
            case SDLK_LEFT:    (void)pressed; break;
            case SDLK_RIGHT:   (void)pressed; break;
            // Row 2: Z, X, Backspace, Enter
            case SDLK_z:       (void)pressed; break;
            case SDLK_x:       (void)pressed; break;
            case SDLK_BACKSPACE: (void)pressed; break;
            case SDLK_RETURN:  (void)pressed; break;
            default: break;
            }
            break;
        }
        }
    }
}

static void sdl_destroy(Frontend* fe)
{
    SDLPriv* priv = fe->priv;
    if (priv->texture)  SDL_DestroyTexture(priv->texture);
    if (priv->renderer) SDL_DestroyRenderer(priv->renderer);
    if (priv->window)   SDL_DestroyWindow(priv->window);
    SDL_Quit();
    free(priv);
}

Frontend* frontend_sdl_create(void)
{
    Frontend* fe = calloc(1, sizeof(Frontend));
    SDLPriv* priv = calloc(1, sizeof(SDLPriv));

    fe->priv = priv;
    fe->init = sdl_init;
    fe->render = sdl_render;
    fe->poll_events = sdl_poll_events;
    fe->destroy = sdl_destroy;

    return fe;
}
