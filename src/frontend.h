#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct gb_frontend gb_frontend;
typedef struct gb_bus gb_bus;
typedef struct gb_apu gb_apu;

typedef enum {
    GB_HOTKEY_SAVE_STATE, // Quick-save the full machine state (F5)
    GB_HOTKEY_LOAD_STATE, // Restore the last quick-save (F9)
} gb_hotkey;

struct gb_frontend {
    void* priv;

    bool (*init)(gb_frontend* fe, int width, int height);
    void (*render)(gb_frontend* fe, const uint32_t* buffer, int width, int height);
    void (*poll_events)(gb_frontend* fe, gb_bus* bus, bool* running);
    void (*destroy)(gb_frontend* fe);

    // Host hooks: frontends emit Hotkey actions; the application decides
    // what they do. on_hotkey may be NULL.
    void* hotkey_ctx;
    void (*on_hotkey)(void* ctx, gb_hotkey key);
};

//@macro GBEMU_STRIP_PREFIX
//@desc Define this before including any gbemu header to alias every prefixed symbol to its unprefixed spelling, so a program that links nothing else can keep writing `CPU`, `ppu_init` or `SCREEN_WIDTH` while the library itself still exports the `gb_`/`GB_` names. This deliberately gives up namespacing: it redefines very common words (`loop`, `version`, `STR`) process-wide, so never enable it alongside another library or code of your own that uses those names. Prefer passing it on the command line (-DGBEMU_STRIP_PREFIX) over defining it in a source file, so it cannot leak into unrelated translation units.
#ifdef GBEMU_STRIP_PREFIX
    //@type Frontend
    #define Frontend gb_frontend

    //@type Hotkey
    #define Hotkey gb_hotkey

    // gb_bus/gb_apu are only forward-declared here (the vtable passes them to
    // poll_events); gbemu.h repeats these aliases so that including either
    // header alone gives you the same short names. Redefining them with an
    // identical replacement list is well defined.
    //@type Bus
    #define Bus gb_bus

    //@type APU
    #define APU gb_apu

    //@const HOTKEY_SAVE_STATE
    #define HOTKEY_SAVE_STATE GB_HOTKEY_SAVE_STATE

    //@const HOTKEY_LOAD_STATE
    #define HOTKEY_LOAD_STATE GB_HOTKEY_LOAD_STATE
#endif // GBEMU_STRIP_PREFIX

#endif // FRONTEND_H

