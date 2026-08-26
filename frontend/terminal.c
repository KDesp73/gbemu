#include "emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#define KEY_HOLD_FRAMES 5

typedef struct {
    struct termios orig_termios;
    int width;
    int height;
    // Per-button hold timers (indexed by button enum below)
    uint8_t hold[8];
} TermPriv;

// Button indices into hold[]
enum {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
    BTN_A, BTN_B, BTN_SEL, BTN_START,
};

static volatile sig_atomic_t got_sigint = 0;

static void sigint_handler(int sig) { (void)sig; got_sigint = 1; }

static bool term_init(Frontend* fe, int width, int height)
{
    TermPriv* priv = fe->priv;
    priv->width = width;
    priv->height = height;
    memset(priv->hold, 0, sizeof(priv->hold));

    if (tcgetattr(STDIN_FILENO, &priv->orig_termios) < 0)
        return false;

    struct termios raw = priv->orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
        return false;

    struct sigaction sa = {0};
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    // Hide cursor, clear screen, disable wrap
    printf("\033[?25l\033[?7l\033[2J\033[H");
    fflush(stdout);
    return true;
}

static void term_render(Frontend* fe, const uint32_t* buffer,
                         int width, int height)
{
    (void)fe;
    // Max per-row: 160 pixels * ~42 bytes each + reset + \r\n < 8192
    char rowbuf[8192];
    int pos;

    printf("\033[H");

    for (int y = 0; y < height - 1; y += 2) {
        pos = 0;
        for (int x = 0; x < width; x++) {
            uint32_t top = buffer[y * width + x];
            uint32_t bot = buffer[(y + 1) * width + x];

            int r = snprintf(rowbuf + pos, sizeof(rowbuf) - pos,
                "\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm\u2580",
                (top >> 16) & 0xFF, (top >> 8) & 0xFF, top & 0xFF,
                (bot >> 16) & 0xFF, (bot >> 8) & 0xFF, bot & 0xFF);
            if (r > 0 && pos + r < (int)sizeof(rowbuf))
                pos += r;
            else
                break;
        }
        pos += snprintf(rowbuf + pos, sizeof(rowbuf) - pos, "\033[0m\r\n");
        fwrite(rowbuf, 1, pos, stdout);
    }
    fflush(stdout);
}

static void press_btn(TermPriv* priv, Bus* bus, int btn)
{
    priv->hold[btn] = KEY_HOLD_FRAMES;
    switch (btn) {
    case BTN_UP:    bus->joypad_dpad    &= ~0x04; break;
    case BTN_DOWN:  bus->joypad_dpad    &= ~0x08; break;
    case BTN_LEFT:  bus->joypad_dpad    &= ~0x02; break;
    case BTN_RIGHT: bus->joypad_dpad    &= ~0x01; break;
    case BTN_A:     bus->joypad_buttons &= ~0x01; break;
    case BTN_B:     bus->joypad_buttons &= ~0x02; break;
    case BTN_SEL:   bus->joypad_buttons &= ~0x04; break;
    case BTN_START: bus->joypad_buttons &= ~0x08; break;
    }
}

static void release_btn(TermPriv* priv, Bus* bus, int btn)
{
    (void)priv;
    switch (btn) {
    case BTN_UP:    bus->joypad_dpad    |= 0x04; break;
    case BTN_DOWN:  bus->joypad_dpad    |= 0x08; break;
    case BTN_LEFT:  bus->joypad_dpad    |= 0x02; break;
    case BTN_RIGHT: bus->joypad_dpad    |= 0x01; break;
    case BTN_A:     bus->joypad_buttons |= 0x01; break;
    case BTN_B:     bus->joypad_buttons |= 0x02; break;
    case BTN_SEL:   bus->joypad_buttons |= 0x04; break;
    case BTN_START: bus->joypad_buttons |= 0x08; break;
    }
}

static void term_poll_events(Frontend* fe, Bus* bus, bool* running)
{
    TermPriv* priv = fe->priv;
    uint8_t buf[64];
    ssize_t n;

    if (got_sigint) { *running = false; return; }

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == 0x1B) {
                if (i + 1 < n && buf[i + 1] == '[') {
                    if (i + 2 < n) {
                        switch (buf[i + 2]) {
                        case 'A': press_btn(priv, bus, BTN_UP);    break;
                        case 'B': press_btn(priv, bus, BTN_DOWN);  break;
                        case 'C': press_btn(priv, bus, BTN_RIGHT); break;
                        case 'D': press_btn(priv, bus, BTN_LEFT);  break;
                        }
                        i += 2;
                    } else { i++; }
                } else {
                    *running = false; return;
                }
            } else if (buf[i] == 'q' || buf[i] == 'Q') {
                *running = false; return;
            } else {
                switch (buf[i]) {
                case 'z': case 'Z': press_btn(priv, bus, BTN_A);     break;
                case 'x': case 'X': press_btn(priv, bus, BTN_B);     break;
                case 0x7F:          press_btn(priv, bus, BTN_SEL);   break;
                case '\n': case '\r': press_btn(priv, bus, BTN_START); break;
                }
            }
        }
    }

    // Tick down hold timers; release expired keys
    for (int i = 0; i < 8; i++) {
        if (priv->hold[i] > 0) {
            priv->hold[i]--;
            if (priv->hold[i] == 0)
                release_btn(priv, bus, i);
        }
    }

    bus->joypad_interrupt = true;
}

static void term_destroy(Frontend* fe)
{
    TermPriv* priv = fe->priv;
    printf("\033[?25h\033[?7h\033[0m\033[2J\033[H");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &priv->orig_termios);
    free(priv);
}

Frontend* frontend_terminal_create(void)
{
    Frontend* fe = calloc(1, sizeof(Frontend));
    TermPriv* priv = calloc(1, sizeof(TermPriv));

    fe->priv = priv;
    fe->init = term_init;
    fe->render = term_render;
    fe->poll_events = term_poll_events;
    fe->destroy = term_destroy;

    return fe;
}
