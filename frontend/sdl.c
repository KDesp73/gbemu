#include "emu.h"
#include <stdlib.h>
#include <SDL3/SDL.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int scale;
    APU* apu;
    SDL_AudioStream* audio_stream;
} SDLPriv;

static void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream,
                                   int additional_amount, int total_amount)
{
    (void)total_amount;
    SDLPriv* priv = userdata;
    int samples_needed = additional_amount / sizeof(float);
    float buf[1024];
    int filled = 0;
    while (filled < samples_needed) {
        int chunk = samples_needed - filled;
        if (chunk > 1024) chunk = 1024;
        for (int i = 0; i < chunk; i++)
            buf[i] = apu_buf_pop(priv->apu);
        SDL_PutAudioStreamData(stream, buf, chunk * sizeof(float));
        filled += chunk;
    }
}

static bool sdl_init(Frontend* fe, int width, int height)
{
    SDLPriv* priv = fe->priv;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "[ERR] SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    const char* scale_env = getenv("EMU_SCALE");
    priv->scale = scale_env ? atoi(scale_env) : 4;
    if (priv->scale < 1) priv->scale = 1;

    priv->window = SDL_CreateWindow(
        "EMU",
        width * priv->scale, height * priv->scale,
        0
    );
    if (!priv->window) {
        fprintf(stderr, "[ERR] SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }

    priv->renderer = SDL_CreateRenderer(priv->window, NULL);
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

    // --- Audio ---
    SDL_AudioSpec spec = {
        .freq     = APU_SAMPLE_RATE,
        .format   = SDL_AUDIO_F32,
        .channels = 1,
    };
    priv->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                   &spec, audio_callback, priv);
    if (!priv->audio_stream) {
        fprintf(stderr, "[ERR] SDL_OpenAudioDeviceStream: %s\n", SDL_GetError());
        return false;
    }
    SDL_ResumeAudioStreamDevice(priv->audio_stream);

    return true;
}

static void sdl_render(Frontend* fe, const uint32_t* buffer,
                       int width, int height)
{
    SDLPriv* priv = fe->priv;

    SDL_UpdateTexture(priv->texture, NULL, buffer, width * sizeof(uint32_t));
    SDL_RenderTexture(priv->renderer, priv->texture, NULL, NULL);
    SDL_RenderPresent(priv->renderer);
}

static void sdl_poll_events(Frontend* fe, Bus* bus, bool* running)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            *running = false;
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            *running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
            switch (event.key.key) {
            // D-pad: bits 3-0 of joypad_dpad (active-low)
            case SDLK_RIGHT: bus->joypad_dpad = pressed ? (bus->joypad_dpad & ~0x01) : (bus->joypad_dpad | 0x01); break;
            case SDLK_LEFT:  bus->joypad_dpad = pressed ? (bus->joypad_dpad & ~0x02) : (bus->joypad_dpad | 0x02); break;
            case SDLK_UP:    bus->joypad_dpad = pressed ? (bus->joypad_dpad & ~0x04) : (bus->joypad_dpad | 0x04); break;
            case SDLK_DOWN:  bus->joypad_dpad = pressed ? (bus->joypad_dpad & ~0x08) : (bus->joypad_dpad | 0x08); break;
            // Face buttons: bits 3-0 of joypad_buttons (active-low)
            case SDLK_Z:       bus->joypad_buttons = pressed ? (bus->joypad_buttons & ~0x01) : (bus->joypad_buttons | 0x01); break; // A
            case SDLK_X:       bus->joypad_buttons = pressed ? (bus->joypad_buttons & ~0x02) : (bus->joypad_buttons | 0x02); break; // B
            case SDLK_BACKSPACE: bus->joypad_buttons = pressed ? (bus->joypad_buttons & ~0x04) : (bus->joypad_buttons | 0x04); break; // Select
            case SDLK_RETURN:  bus->joypad_buttons = pressed ? (bus->joypad_buttons & ~0x08) : (bus->joypad_buttons | 0x08); break; // Start
            // Host hotkeys (fire on press only)
            case SDLK_F5: if (pressed && fe->on_hotkey) fe->on_hotkey(fe->hotkey_ctx, HOTKEY_SAVE_STATE); break;
            case SDLK_F9: if (pressed && fe->on_hotkey) fe->on_hotkey(fe->hotkey_ctx, HOTKEY_LOAD_STATE); break;
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
    if (priv->audio_stream) {
        SDL_DestroyAudioStream(priv->audio_stream);
    }
    if (priv->texture)  SDL_DestroyTexture(priv->texture);
    if (priv->renderer) SDL_DestroyRenderer(priv->renderer);
    if (priv->window)   SDL_DestroyWindow(priv->window);
    SDL_Quit();
    free(priv);
}

Frontend* frontend_sdl_create(APU* apu)
{
    Frontend* fe = calloc(1, sizeof(Frontend));
    SDLPriv* priv = calloc(1, sizeof(SDLPriv));
    priv->apu = apu;

    fe->priv = priv;
    fe->init = sdl_init;
    fe->render = sdl_render;
    fe->poll_events = sdl_poll_events;
    fe->destroy = sdl_destroy;

    return fe;
}
