#include "gbemu.h"
#include <stdlib.h>
#include <SDL3/SDL.h>

#ifdef GBEMU_DEBUGGER
#include "imgui_backends.h"
#include "debugger.h"
#endif

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int scale;
    gb_apu* apu;
    SDL_AudioStream* audio_stream;
#ifdef GBEMU_DEBUGGER
    SDL_Window* debug_window;
    SDL_Renderer* debug_renderer;
    SDL_WindowID debug_window_id;
    bool debug_visible;
#endif
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
            buf[i] = gb_apu_buf_pop(priv->apu);
        SDL_PutAudioStreamData(stream, buf, chunk * sizeof(float));
        filled += chunk;
    }
}

static bool sdl_init(gb_frontend* fe, int width, int height)
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
        .freq     = GB_APU_SAMPLE_RATE,
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

static void sdl_render(gb_frontend* fe, const uint32_t* buffer,
                       int width, int height)
{
    SDLPriv* priv = fe->priv;

    SDL_UpdateTexture(priv->texture, NULL, buffer, width * sizeof(uint32_t));
    SDL_RenderTexture(priv->renderer, priv->texture, NULL, NULL);
    SDL_RenderPresent(priv->renderer);
}

#ifdef GBEMU_DEBUGGER
static bool sdl_debug_ensure(SDLPriv* priv)
{
    if (priv->debug_window) return true;

    priv->debug_window = SDL_CreateWindow("GB Debugger", 780, 660, SDL_WINDOW_RESIZABLE);
    if (!priv->debug_window) {
        fprintf(stderr, "[ERR] Debug SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }
    priv->debug_renderer = SDL_CreateRenderer(priv->debug_window, NULL);
    if (!priv->debug_renderer) {
        fprintf(stderr, "[ERR] Debug SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(priv->debug_window);
        priv->debug_window = NULL;
        return false;
    }
    if (!gb_imgui_init(priv->debug_window, priv->debug_renderer)) {
        fprintf(stderr, "[ERR] ImGui initialization failed\n");
        SDL_DestroyRenderer(priv->debug_renderer);
        SDL_DestroyWindow(priv->debug_window);
        priv->debug_window = NULL;
        priv->debug_renderer = NULL;
        return false;
    }
    priv->debug_window_id = SDL_GetWindowID(priv->debug_window);
    priv->debug_visible = true;
    return true;
}

static SDL_WindowID event_window_id(const SDL_Event* ev)
{
    switch (ev->type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:          return ev->key.windowID;
    case SDL_EVENT_TEXT_INPUT:      return ev->text.windowID;
    case SDL_EVENT_MOUSE_MOTION:    return ev->motion.windowID;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: return ev->button.windowID;
    case SDL_EVENT_MOUSE_WHEEL:     return ev->wheel.windowID;
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                                    return ev->window.windowID;
    default:                        return 0;
    }
}
#endif

static void sdl_poll_events(gb_frontend* fe, gb_bus* bus, bool* running)
{
    SDLPriv* priv = fe->priv;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Debugger/host hotkeys fire no matter which window has focus.
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            switch (event.key.key) {
            case SDLK_F1:
#ifdef GBEMU_DEBUGGER
                if (!priv->debug_window) sdl_debug_ensure(priv);
                if (priv->debug_window)
                    priv->debug_visible = !priv->debug_visible;
#endif
                continue;
            case SDLK_F6:
                if (fe->on_hotkey)
                    fe->on_hotkey(fe->hotkey_ctx, GB_HOTKEY_DEBUG_PAUSE);
                continue;
            case SDLK_F5:
                if (fe->on_hotkey)
                    fe->on_hotkey(fe->hotkey_ctx, GB_HOTKEY_SAVE_STATE);
                continue;
            case SDLK_F9:
                if (fe->on_hotkey)
                    fe->on_hotkey(fe->hotkey_ctx, GB_HOTKEY_LOAD_STATE);
                continue;
            default: break;
            }
        }

#ifdef GBEMU_DEBUGGER
        // Events meant for the debug window go to ImGui only.
        if (priv->debug_window && event_window_id(&event) == priv->debug_window_id) {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                priv->debug_visible = false;
                continue;
            }
            gb_imgui_process_event(&event);
            continue;
        }
#endif

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
            default: break;
            }
            if (pressed) bus->joypad_interrupt = true;
            break;
        }
        }
    }
}

#ifdef GBEMU_DEBUGGER
static void sdl_render_debug(gb_frontend* fe)
{
    SDLPriv* priv = fe->priv;
    if (!priv->debug_window || !priv->debug_visible) return;
    gb_imgui_new_frame();
    debug_ui_panels(fe->debug);
    gb_imgui_render(priv->debug_renderer);
}
#endif

static void sdl_destroy(gb_frontend* fe)
{
    SDLPriv* priv = fe->priv;
    if (priv->audio_stream) {
        SDL_DestroyAudioStream(priv->audio_stream);
    }
#ifdef GBEMU_DEBUGGER
    if (priv->debug_window) {
        gb_imgui_shutdown();
        if (priv->debug_renderer) SDL_DestroyRenderer(priv->debug_renderer);
        SDL_DestroyWindow(priv->debug_window);
    }
#endif
    if (priv->texture)  SDL_DestroyTexture(priv->texture);
    if (priv->renderer) SDL_DestroyRenderer(priv->renderer);
    if (priv->window)   SDL_DestroyWindow(priv->window);
    SDL_Quit();
    free(priv);
}

gb_frontend* frontend_sdl_create(gb_apu* apu)
{
    gb_frontend* fe = calloc(1, sizeof(gb_frontend));
    SDLPriv* priv = calloc(1, sizeof(SDLPriv));
    priv->apu = apu;

    fe->priv = priv;
    fe->init = sdl_init;
    fe->render = sdl_render;
    fe->poll_events = sdl_poll_events;
    fe->destroy = sdl_destroy;
#ifdef GBEMU_DEBUGGER
    fe->render_debug = sdl_render_debug;
#endif

    return fe;
}
