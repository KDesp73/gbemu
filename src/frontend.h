#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Frontend Frontend;
typedef struct Bus Bus;
typedef struct APU APU;

struct Frontend {
    void* priv;

    bool (*init)(Frontend* fe, int width, int height);
    void (*render)(Frontend* fe, const uint32_t* buffer, int width, int height);
    void (*poll_events)(Frontend* fe, Bus* bus, bool* running);
    void (*destroy)(Frontend* fe);
};

#endif // FRONTEND_H
