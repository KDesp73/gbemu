#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Frontend Frontend;
typedef struct Bus Bus;
typedef struct APU APU;

typedef enum {
    HOTKEY_SAVE_STATE, // Quick-save the full machine state (F5)
    HOTKEY_LOAD_STATE, // Restore the last quick-save (F9)
} Hotkey;

struct Frontend {
    void* priv;

    bool (*init)(Frontend* fe, int width, int height);
    void (*render)(Frontend* fe, const uint32_t* buffer, int width, int height);
    void (*poll_events)(Frontend* fe, Bus* bus, bool* running);
    void (*destroy)(Frontend* fe);

    // Host hooks: frontends emit Hotkey actions; the application decides
    // what they do. on_hotkey may be NULL.
    void* hotkey_ctx;
    void (*on_hotkey)(void* ctx, Hotkey key);
};

#endif // FRONTEND_H

