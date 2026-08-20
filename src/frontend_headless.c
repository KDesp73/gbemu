#include "emu.h"
#include <stdlib.h>

static bool headless_init(Frontend* fe, int width, int height)
{
    (void)fe; (void)width; (void)height;
    return true;
}

static void headless_render(Frontend* fe, const uint32_t* buffer,
                            int width, int height)
{
    (void)fe; (void)buffer; (void)width; (void)height;
}

static void headless_poll_events(Frontend* fe, Bus* bus, bool* running)
{
    (void)fe; (void)bus; (void)running;
}

static void headless_destroy(Frontend* fe)
{
    free(fe->priv);
}

Frontend* frontend_headless_create(void)
{
    Frontend* fe = calloc(1, sizeof(Frontend));
    fe->priv = NULL;
    fe->init = headless_init;
    fe->render = headless_render;
    fe->poll_events = headless_poll_events;
    fe->destroy = headless_destroy;
    return fe;
}
