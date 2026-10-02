// ESP-IDF frontend: SPI TFT display (esp_lcd), GPIO buttons, optional I2S audio.
//
// This file is meant to be compiled inside an external ESP-IDF (>= 5.0)
// project: register it alongside your main component sources together with
// the core in src/. It requires the esp_lcd, driver (gpio/i2s) and
// freertos components.
//
// All hardware assumptions are compile-time macros so the frontend can be
// adapted to any board without touching the logic below. Defaults target a
// generic ESP32 DevKit + 240x240 ST7789 SPI panel + I2S DAC.

#include "gbemu.h"

#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Display wiring ---------------------------------------------------------
#ifndef EMU_LCD_SPI_HOST
#define EMU_LCD_SPI_HOST SPI2_HOST
#endif
#ifndef EMU_LCD_PIN_SCLK
#define EMU_LCD_PIN_SCLK 18
#endif
#ifndef EMU_LCD_PIN_MOSI
#define EMU_LCD_PIN_MOSI 23
#endif
#ifndef EMU_LCD_PIN_CS
#define EMU_LCD_PIN_CS   15
#endif
#ifndef EMU_LCD_PIN_DC
#define EMU_LCD_PIN_DC    2
#endif
#ifndef EMU_LCD_PIN_RST
#define EMU_LCD_PIN_RST   4
#endif
#ifndef EMU_LCD_PIN_BL
#define EMU_LCD_PIN_BL   21
#endif
#ifndef EMU_LCD_BL_ACTIVE_LOW
#define EMU_LCD_BL_ACTIVE_LOW 0
#endif

// Panel geometry. Defaults fit a 240x240 ST7789; the 160x144 frame is
// centered via the panel gap. Define EMU_LCD_WIDTH/HGT/OFF_X/OFF_Y for other
// panels (e.g. 160x128 ST7735: OFF_X=0, OFF_Y=0).
#ifndef EMU_LCD_PANEL_ST7789
#define EMU_LCD_PANEL_ST7789 1
#endif
#ifndef EMU_LCD_WIDTH
#define EMU_LCD_WIDTH  240
#endif
#ifndef EMU_LCD_HEIGHT
#define EMU_LCD_HEIGHT 240
#endif
#ifndef EMU_LCD_OFF_X
#define EMU_LCD_OFF_X ((EMU_LCD_WIDTH - GB_SCREEN_WIDTH) / 2)
#endif
#ifndef EMU_LCD_OFF_Y
#define EMU_LCD_OFF_Y ((EMU_LCD_HEIGHT - GB_SCREEN_HEIGHT) / 2)
#endif
#ifndef EMU_LCD_SPI_CLOCK_HZ
#define EMU_LCD_SPI_CLOCK_HZ (80 * 1000 * 1000) // drop to 40MHz for flaky wiring
#endif
#ifndef EMU_LCD_MIRROR_X
#define EMU_LCD_MIRROR_X false
#endif
#ifndef EMU_LCD_MIRROR_Y
#define EMU_LCD_MIRROR_Y false
#endif
#ifndef EMU_LCD_COLOR_INVERT
#define EMU_LCD_COLOR_INVERT 0 // many ST7789 modules need 1
#endif

// --- Button wiring (active-low, internal pull-ups) ---------------------------
#ifndef EMU_BTN_UP
#define EMU_BTN_UP     32
#endif
#ifndef EMU_BTN_DOWN
#define EMU_BTN_DOWN   33
#endif
#ifndef EMU_BTN_LEFT
#define EMU_BTN_LEFT   25
#endif
#ifndef EMU_BTN_RIGHT
#define EMU_BTN_RIGHT  26
#endif
#ifndef EMU_BTN_A
#define EMU_BTN_A      27
#endif
#ifndef EMU_BTN_B
#define EMU_BTN_B      14
#endif
#ifndef EMU_BTN_SELECT
#define EMU_BTN_SELECT 13
#endif
#ifndef EMU_BTN_START
#define EMU_BTN_START  19 // GPIO 16/17 collide with PSRAM on WROVER boards
#endif

// START+SELECT held together: SAVE fires at ~0.5s, LOAD at ~2.5s
#ifndef EMU_HOTKEY_SAVE_FRAMES
#define EMU_HOTKEY_SAVE_FRAMES 30
#endif
#ifndef EMU_HOTKEY_LOAD_FRAMES
#define EMU_HOTKEY_LOAD_FRAMES 150
#endif

// --- Audio wiring (set EMU_ENABLE_AUDIO 0 for headless/DAC-less boards) -----
#ifndef EMU_ENABLE_AUDIO
#define EMU_ENABLE_AUDIO 1
#endif
#ifndef EMU_I2S_PIN_BCLK
#define EMU_I2S_PIN_BCLK 27
#endif
#ifndef EMU_I2S_PIN_LRCK
#define EMU_I2S_PIN_LRCK 26
#endif
#ifndef EMU_I2S_PIN_DOUT
#define EMU_I2S_PIN_DOUT 25
#endif
#ifndef EMU_AUDIO_TASK_CORE
#define EMU_AUDIO_TASK_CORE 1
#endif

typedef struct {
    gb_apu* apu;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;

    uint16_t* pixels;       // RGB565 (byte-swapped), DMA-capable, w*h

    i2s_chan_handle_t i2s_tx;
    TaskHandle_t audio_task;
    volatile bool audio_quit;

    int combo_frames;       // consecutive frames START+SELECT were held
} ESPPriv;

static void audio_task_fn(void* arg);

static bool btn_down(int gpio)
{
    return gpio_get_level(gpio) == 0;
}

static bool esp32_init(gb_frontend* fe, int width, int height)
{
    ESPPriv* priv = fe->priv;

    // --- Display ---
    spi_bus_config_t buscfg = {
        .mosi_io_num = EMU_LCD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = EMU_LCD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EMU_LCD_WIDTH * EMU_LCD_HEIGHT * sizeof(uint16_t),
    };
    if (spi_bus_initialize(EMU_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) {
        fprintf(stderr, "[ERR] spi_bus_initialize\n");
        return false;
    }

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = EMU_LCD_PIN_CS,
        .dc_gpio_num = EMU_LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = EMU_LCD_SPI_CLOCK_HZ,
        .trans_queue_depth = 4,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EMU_LCD_SPI_HOST,
                                 &io_cfg, &priv->io) != ESP_OK) {
        fprintf(stderr, "[ERR] esp_lcd_new_panel_io_spi\n");
        return false;
    }

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = EMU_LCD_PIN_RST,
    };
#if defined(EMU_LCD_PANEL_ST7735)
    if (esp_lcd_new_panel_st7735(priv->io, &panel_cfg, &priv->panel) != ESP_OK)
#else
    if (esp_lcd_new_panel_st7789(priv->io, &panel_cfg, &priv->panel) != ESP_OK)
#endif
    {
        fprintf(stderr, "[ERR] esp_lcd_new_panel\n");
        return false;
    }

    esp_lcd_panel_reset(priv->panel);
    esp_lcd_panel_init(priv->panel);
    esp_lcd_panel_swap_xy(priv->panel, false);
    esp_lcd_panel_mirror(priv->panel, EMU_LCD_MIRROR_X, EMU_LCD_MIRROR_Y);
    esp_lcd_panel_set_gap(priv->panel, EMU_LCD_OFF_X, EMU_LCD_OFF_Y);
#if EMU_LCD_COLOR_INVERT
    esp_lcd_panel_invert_color(priv->panel, true);
#endif
    esp_lcd_panel_disp_on_off(priv->panel, true);

    gpio_config_t bl_cfg = {
        .pin_bit_mask = 1ULL << EMU_LCD_PIN_BL,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(EMU_LCD_PIN_BL, EMU_LCD_BL_ACTIVE_LOW ? 0 : 1);

    priv->pixels = heap_caps_malloc(
        width * height * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!priv->pixels) {
        fprintf(stderr, "[ERR] framebuffer alloc (%dx%d RGB565)\n", width, height);
        return false;
    }

    // --- Input ---
    const gpio_num_t btn_pins[] = {
        EMU_BTN_UP, EMU_BTN_DOWN, EMU_BTN_LEFT, EMU_BTN_RIGHT,
        EMU_BTN_A, EMU_BTN_B, EMU_BTN_SELECT, EMU_BTN_START,
    };
    uint64_t mask = 0;
    for (size_t i = 0; i < sizeof(btn_pins) / sizeof(btn_pins[0]); i++)
        mask |= 1ULL << btn_pins[i];
    gpio_config_t in_cfg = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&in_cfg);

    // --- Audio ---
#if EMU_ENABLE_AUDIO
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    if (i2s_new_channel(&chan_cfg, &priv->i2s_tx, NULL) == ESP_OK) {
        i2s_std_config_t std_cfg = {
            .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(GB_APU_SAMPLE_RATE),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
            .gpio_cfg = {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = EMU_I2S_PIN_BCLK,
                .ws   = EMU_I2S_PIN_LRCK,
                .dout = EMU_I2S_PIN_DOUT,
                .din  = I2S_GPIO_UNUSED,
            },
        };
        std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
        if (i2s_channel_init_std_mode(priv->i2s_tx, &std_cfg) == ESP_OK &&
            i2s_channel_enable(priv->i2s_tx) == ESP_OK) {
            BaseType_t ok = xTaskCreatePinnedToCore(
                audio_task_fn, "gb-audio", 3072, priv, 6,
                &priv->audio_task, EMU_AUDIO_TASK_CORE);
            if (ok != pdPASS) priv->audio_task = NULL;
        } else {
            fprintf(stderr, "[WARN] i2s init failed, audio disabled\n");
            i2s_del_channel(priv->i2s_tx);
            priv->i2s_tx = NULL;
        }
    } else {
        fprintf(stderr, "[WARN] i2s channel alloc failed, audio disabled\n");
    }
#endif

    return true;
}

static void esp32_render(gb_frontend* fe, const uint32_t* buffer,
                         int width, int height)
{
    ESPPriv* priv = fe->priv;

    // XRGB8888 -> RGB565 with swapped bytes (SPI panels shift out MSB first)
    int total = width * height;
    for (int i = 0; i < total; i++) {
        uint32_t px = buffer[i];
        uint8_t r = (px >> 16) & 0xFF;
        uint8_t g = (px >> 8) & 0xFF;
        uint8_t b = px & 0xFF;
        uint16_t rgb565 = (uint16_t)(((r & 0xF8) << 8) |
                                     ((g & 0xFC) << 3) | (b >> 3));
        priv->pixels[i] = (uint16_t)((rgb565 << 8) | (rgb565 >> 8));
    }

    esp_lcd_panel_draw_bitmap(priv->panel, 0, 0, width, height, priv->pixels);
}

static void esp32_poll_events(gb_frontend* fe, gb_bus* bus, bool* running)
{
    ESPPriv* priv = fe->priv;
    (void)running;

    uint8_t old_dpad = bus->joypad_dpad;
    uint8_t old_buttons = bus->joypad_buttons;

    // Same active-low bit layout as frontend/sdl.c:
    // dpad bits 3-0: Down Up Left Right; buttons bits 3-0: Start Select B A
    bus->joypad_dpad = (bus->joypad_dpad & ~0x0F)
        | (btn_down(EMU_BTN_RIGHT) ? 0x00 : 0x01)
        | (btn_down(EMU_BTN_LEFT)  ? 0x00 : 0x02)
        | (btn_down(EMU_BTN_UP)    ? 0x00 : 0x04)
        | (btn_down(EMU_BTN_DOWN)  ? 0x00 : 0x08);

    bus->joypad_buttons = (bus->joypad_buttons & ~0x0F)
        | (btn_down(EMU_BTN_A)      ? 0x00 : 0x01)
        | (btn_down(EMU_BTN_B)      ? 0x00 : 0x02)
        | (btn_down(EMU_BTN_SELECT) ? 0x00 : 0x04)
        | (btn_down(EMU_BTN_START)  ? 0x00 : 0x08);

    // Raise joypad interrupt on button press (bit transition from 1 to 0)
    if ((old_dpad & ~bus->joypad_dpad & 0x0F) || (old_buttons & ~bus->joypad_buttons & 0x0F))
        bus->joypad_interrupt = true;

    if (fe->on_hotkey) {
        bool combo = btn_down(EMU_BTN_START) && btn_down(EMU_BTN_SELECT);
        if (combo) {
            priv->combo_frames++;
            if (priv->combo_frames == EMU_HOTKEY_SAVE_FRAMES)
                fe->on_hotkey(fe->hotkey_ctx, GB_HOTKEY_SAVE_STATE);
            else if (priv->combo_frames == EMU_HOTKEY_LOAD_FRAMES)
                fe->on_hotkey(fe->hotkey_ctx, GB_HOTKEY_LOAD_STATE);
        } else {
            priv->combo_frames = 0;
        }
    }
}

static void esp32_destroy(gb_frontend* fe)
{
    ESPPriv* priv = fe->priv;

#if EMU_ENABLE_AUDIO
    if (priv->audio_task) {
        priv->audio_quit = true;
        vTaskDelay(pdMS_TO_TICKS(200));
        vTaskDelete(priv->audio_task);
    }
    if (priv->i2s_tx) {
        i2s_channel_disable(priv->i2s_tx);
        i2s_del_channel(priv->i2s_tx);
    }
#endif
    if (priv->panel) esp_lcd_panel_del(priv->panel);
    if (priv->io)    esp_lcd_panel_io_del(priv->io);
    spi_bus_free(EMU_LCD_SPI_HOST);
    free(priv->pixels);
    free(priv);
    free(fe);
}

static void audio_task_fn(void* arg)
{
    ESPPriv* priv = arg;
    enum { CHUNK = 256 };
    int16_t samples[CHUNK];

    while (!priv->audio_quit) {
        for (int i = 0; i < CHUNK; i++) {
            float s = gb_apu_buf_pop(priv->apu);
            if (s > 1.0f) s = 1.0f;
            if (s < -1.0f) s = -1.0f;
            samples[i] = (int16_t)(s * 32767.0f);
        }
        size_t written = 0;
        // Blocking write paces the task to the sample rate
        if (i2s_channel_write(priv->i2s_tx, samples, sizeof(samples),
                              &written, pdMS_TO_TICKS(100)) != ESP_OK)
            vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

gb_frontend* frontend_esp32_create(gb_apu* apu)
{
    gb_frontend* fe = calloc(1, sizeof(gb_frontend));
    ESPPriv* priv = calloc(1, sizeof(ESPPriv));
    priv->apu = apu;

    fe->priv = priv;
    fe->init = esp32_init;
    fe->render = esp32_render;
    fe->poll_events = esp32_poll_events;
    fe->destroy = esp32_destroy;

    return fe;
}
