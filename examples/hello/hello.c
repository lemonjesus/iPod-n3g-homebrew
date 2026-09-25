/*
 * Hello - starter game for the iPod nano 3G eApp SDK.
 *
 * A square you steer with the click wheel. Prev/Next move it up and down,
 * the center button changes its colour, Play/Pause resets it, Menu quits.
 * The five boxes along the bottom light up while their button is held.
 */
#include "eapp.h"
#include "eapp_gles.h"
#include "eapp_2d.h"

#define SIZE 24
#define BTN_COUNT 5

static const uint32_t colours[] = { 0xFFD000, 0x40C0FF, 0xFF4060, 0x60FF60 };

static int x, y, colour;
static int quit;
static uint8_t held[BTN_COUNT + 1]; 

// Run once after the app is loaded before anything is drawn. There is no GL context yet.
void eapp_init(void) {
    x = (EAPP_SCREEN_W - SIZE) / 2;
    y = (EAPP_SCREEN_H - SIZE) / 2;
}

// Handle input (buttons or clickwheel)
static void handle_input(eapp_os_to_game* os) {
    for (eapp_input_event* ev = os->input; ev; ev = ev->next) {
        if (ev->button >= 1 && ev->button <= BTN_COUNT)
            held[ev->button] = (ev->action == EAPP_ACT_DOWN);

        if (ev->action != EAPP_ACT_UP)
            continue;
        switch (ev->button) {
        case EAPP_BTN_MENU:   quit = 1; break;
        case EAPP_BTN_SELECT: colour = (colour + 1) % 4; break;
        case EAPP_BTN_PREV:   y -= SIZE; break;
        case EAPP_BTN_NEXT:   y += SIZE; break;
        case EAPP_BTN_PLAY:   eapp_init(); break;
        }
    }

    int delta = 0;
    uint32_t extra;
    eapp_read_wheel(&delta, &extra);   /* clockwise is positive */
    x += delta * 4;

    if (x < 0) x = 0;
    if (x > EAPP_SCREEN_W - SIZE) x = EAPP_SCREEN_W - SIZE;
    if (y < 0) y = 0;
    if (y > EAPP_SCREEN_H - SIZE) y = EAPP_SCREEN_H - SIZE;
}

// This runs on each frame and is where your game logic and drawing go
void eapp_frame(eapp_os_to_game* os, eapp_game_to_os* game) {
    // If the OS asked us to stop, be good and stop. We also keep track of if we want to quit ourselves.
    if (os->command != EAPP_CMD_RUN || quit) {
        game->state = EAPP_STATE_DONE;
        return;
    }

    handle_input(os);

    eapp_gl2d_begin();
    eapp_gl2d_rect(0, 0, EAPP_SCREEN_W, EAPP_SCREEN_H, 0x101828);
    eapp_gl2d_rect(x, y, SIZE, SIZE, colours[colour]);

    for (int i = 1; i <= BTN_COUNT; i++) {
        int bx = 8 + (i - 1) * 48;
        eapp_gl2d_rect(bx, EAPP_SCREEN_H - 20, 40, 12, held[i] ? 0xFFFFFF : 0x304050);
    }

    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
}
