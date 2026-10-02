#include "gbemu.h"
#include <stdlib.h>

static bool headless_init(gb_frontend* fe, int width, int height)
{
    (void)fe; (void)width; (void)height;
    return true;
}

static void headless_render(gb_frontend* fe, const uint32_t* buffer,
                            int width, int height)
{
    (void)fe; (void)buffer; (void)width; (void)height;
}

static void headless_poll_events(gb_frontend* fe, gb_bus* bus, bool* running)
{
    (void)fe; (void)bus; (void)running;
}

static void headless_destroy(gb_frontend* fe)
{
    free(fe->priv);
}

gb_frontend* frontend_headless_create(void)
{
    gb_frontend* fe = calloc(1, sizeof(gb_frontend));
    fe->priv = NULL;
    fe->init = headless_init;
    fe->render = headless_render;
    fe->poll_events = headless_poll_events;
    fe->destroy = headless_destroy;
    return fe;
}
