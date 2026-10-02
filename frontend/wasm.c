#include "gbemu.h"
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

static bool wasm_init(gb_frontend* fe, int width, int height)
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

static void wasm_render(gb_frontend* fe, const uint32_t* buffer,
                        int width, int height)
{
    js_render_to_canvas(buffer, width, height);
}

static void wasm_poll_events(gb_frontend* fe, gb_bus* bus, bool* running)
{
}

static void wasm_destroy(gb_frontend* fe)
{
    free(fe->priv);
}

gb_frontend* frontend_wasm_create(void)
{
    gb_frontend* fe = calloc(1, sizeof(gb_frontend));
    fe->priv = NULL;
    fe->init = wasm_init;
    fe->render = wasm_render;
    fe->poll_events = wasm_poll_events;
    fe->destroy = wasm_destroy;
    return fe;
}
