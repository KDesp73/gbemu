#include "emu.h"
#include <stdlib.h>
#include <emscripten/html5.h>

EM_JS(void, js_render_to_canvas, (const void* buffer, int width, int height), {
    var canvas = document.getElementById('screen');
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    if (!ctx) return;

    var imageData = ctx.createImageData(width, height);
    var data = imageData.data;
    var buf32 = new Uint32Array(Module.HEAPU8.buffer, buffer, width * height);

    for (var i = 0; i < width * height; i++) {
        var pixel = buf32[i];
        data[i * 4]     = (pixel >> 16) & 0xFF;
        data[i * 4 + 1] = (pixel >> 8) & 0xFF;
        data[i * 4 + 2] = pixel & 0xFF;
        data[i * 4 + 3] = 0xFF;
    }

    ctx.putImageData(imageData, 0, 0);
});

static bool wasm_init(Frontend* fe, int width, int height)
{
    EM_ASM({
        var canvas = document.getElementById('screen');
        if (canvas) {
            canvas.width = $0;
            canvas.height = $1;
        }
    }, width, height);
    return true;
}

static void wasm_render(Frontend* fe, const uint32_t* buffer,
                        int width, int height)
{
    js_render_to_canvas(buffer, width, height);
}

static void wasm_poll_events(Frontend* fe, Bus* bus, bool* running)
{
}

static void wasm_destroy(Frontend* fe)
{
    free(fe->priv);
}

Frontend* frontend_wasm_create(void)
{
    Frontend* fe = calloc(1, sizeof(Frontend));
    fe->priv = NULL;
    fe->init = wasm_init;
    fe->render = wasm_render;
    fe->poll_events = wasm_poll_events;
    fe->destroy = wasm_destroy;
    return fe;
}
