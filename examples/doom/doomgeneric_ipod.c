/*
 * doomgeneric_ipod.c - doomgeneric platform layer for the iPod nano 3G eApp SDK.
 *
 * doomgeneric expects to own a main loop; eApps are called once per frame instead.
 * The first eapp_frame runs doomgeneric_Create (all of Doom's startup), every later
 * one runs a single doomgeneric_Tick. doom1.wad is read from the game's own folder.
 *
 * Controls (in game / in the menu):
 *   wheel   turn            / move up and down
 *   Select  fire            / choose
 *   Play    forward         / "yes" to prompts
 *   Next    use (doors)     / slider right
 *   Prev    back            / slider left
 *   Menu    open the menu   / back
 *   hold Menu for 2 s to quit at any time
 */
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "eapp.h"
#include "eapp_gles.h"
#include "eapp_2d.h"

#include "doomgeneric.h"
#include "doomkeys.h"
#include "d_event.h"

extern boolean menuactive;

// Doom's exit()/I_Error lands here (see _exit in ipod_syscalls.c).
jmp_buf ipod_exit_jmp;

static char* doom_argv[] = {
    "doom",
    "-iwad", "doom1.wad",
    "-gfxmode", "rgb565",   // write screen-ready pixels into DG_ScreenBuffer
    "-nogui",
    NULL,
};

static int started, quit;
static unsigned frames;

#define QUIT_HOLD_MS 2000   // hold Menu this long to leave, whatever Doom is doing

// The OS's 1 MHz counter. Doom only ever sleeps while waiting for the next tic (at most
// ~29 ms, inside one eapp_frame), so sleeping is a busy wait.
static uint32_t usec_now(void) {
    uint32_t t;
    misc_GetUsecTimer(&t);
    return t;
}

uint32_t DG_GetTicksMs(void) {
    // Accumulate deltas so the 32-bit microsecond wrap (every ~71 min) is harmless.
    static uint32_t last_us, ms, frac_us;
    uint32_t now = usec_now();
    frac_us += now - last_us;
    last_us = now;
    ms += frac_us / 1000;
    frac_us %= 1000;
    return ms;
}

void DG_SleepMs(uint32_t ms) {
    uint32_t start = usec_now();
    while (usec_now() - start < ms * 1000)
        ;
}

#define KEYQUEUE_SIZE 32
#define WHEEL_TURN    48   // mouse units per wheel step, sets turning speed
#define WHEEL_STEP    4    // wheel steps per menu line

static uint16_t keyqueue[KEYQUEUE_SIZE];
static unsigned key_rd, key_wr;
static uint8_t held_key[EAPP_BTN_PLAY + 1];   // key sent on press, released on release
static int menu_held;
static uint32_t menu_down_ms;
static int wheel_acc;

static void add_key(int pressed, uint8_t key) {
    keyqueue[key_wr++ % KEYQUEUE_SIZE] = (uint16_t)(pressed << 8 | key);
}

int DG_GetKey(int* pressed, unsigned char* key) {
    if (key_rd == key_wr)
        return 0;
    uint16_t k = keyqueue[key_rd++ % KEYQUEUE_SIZE];
    *pressed = k >> 8;
    *key = k & 0xFF;
    return 1;
}

static uint8_t button_key(int button) {
    switch (button) {
    case EAPP_BTN_MENU:   return KEY_ESCAPE;
    case EAPP_BTN_SELECT: return menuactive ? KEY_ENTER : KEY_FIRE;
    case EAPP_BTN_PLAY:   return menuactive ? 'y' : KEY_UPARROW;
    case EAPP_BTN_NEXT:   return menuactive ? KEY_RIGHTARROW : KEY_USE;
    case EAPP_BTN_PREV:   return menuactive ? KEY_LEFTARROW : KEY_DOWNARROW;
    }
    return 0;
}

static void handle_input(eapp_os_to_game* os) {
    for (eapp_input_event* ev = os->input; ev; ev = ev->next) {
        if (ev->button < EAPP_BTN_MENU || ev->button > EAPP_BTN_PLAY)
            continue;
        if (ev->button == EAPP_BTN_MENU) {
            menu_held = ev->action == EAPP_ACT_DOWN;
            if (menu_held)
                menu_down_ms = DG_GetTicksMs();
        }
        if (ev->action == EAPP_ACT_DOWN && !held_key[ev->button]) {
            held_key[ev->button] = button_key(ev->button);
            add_key(1, held_key[ev->button]);
        } else if (ev->action == EAPP_ACT_UP && held_key[ev->button]) {
            add_key(0, held_key[ev->button]);
            held_key[ev->button] = 0;
        }
    }

    int delta = 0;
    uint32_t extra;
    eapp_read_wheel(&delta, &extra);
    if (!delta || !started)
        return;

    if (menuactive) {
        wheel_acc += delta;
        for (; wheel_acc >= WHEEL_STEP; wheel_acc -= WHEEL_STEP) {
            add_key(1, KEY_DOWNARROW);
            add_key(0, KEY_DOWNARROW);
        }
        for (; wheel_acc <= -WHEEL_STEP; wheel_acc += WHEEL_STEP) {
            add_key(1, KEY_UPARROW);
            add_key(0, KEY_UPARROW);
        }
    } else {
        event_t ev = { .type = ev_mouse, .data2 = delta * WHEEL_TURN };
        D_PostEvent(&ev);
    }
}

// Doom writes RGB565 rows RESX (512) pixels apart, filling a 320x200 box at x=96.
// doomgeneric allocates RESX*RESY*4 bytes (it assumes 32-bit pixels); RESY=200 keeps
// that under the OS's 512 KB allocation limit. Each frame is copied straight into the
// OS back buffer (320x240 RGB565), letterboxed, then eglSwapBuffers shows it.
#define DOOM_W   320
#define DOOM_H   200
#define DOOM_X   ((DOOMGENERIC_RESX - DOOM_W) / 2)
#define SCREEN_Y ((EAPP_SCREEN_H - DOOM_H) / 2)

void DG_Init(void) {
    memset(DG_ScreenBuffer, 0, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
}

// Stubs that don't apply here
void DG_SetWindowTitle(const char* title) { (void)title; }
void DG_DrawFrame(void) {}

static void present(eapp_os_to_game* os) {
    // Two pixels per word; all offsets here are multiples of 4 bytes.
    uint32_t* out = (uint32_t*)os->backbuffer;
    int x, y;

    if (frames == 1)
        printf("ipod: back buffer %p\n", (void* )out);

    if (out && ((uintptr_t)out & 3) == 0) {
        for (y = 0; y < EAPP_SCREEN_H; y++) {
            int doom_y = y - SCREEN_Y;
            if (DG_ScreenBuffer && doom_y >= 0 && doom_y < DOOM_H) {
                const uint32_t* in = (const uint32_t*)
                    ((const uint16_t*)DG_ScreenBuffer + doom_y * DOOMGENERIC_RESX + DOOM_X);
                for (x = 0; x < EAPP_SCREEN_W / 2; x++)
                    *out++ = in[x];
            } else {
                for (x = 0; x < EAPP_SCREEN_W / 2; x++)
                    *out++ = 0;
            }
        }
    }

    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
}

// I_Error leaves a red screen (and doom.log, if the OS lets us write it) until Menu is
// pressed, so a failure is distinguishable from a plain quit.
void ipod_log_save(void);

static int failed;

static void show_error(eapp_os_to_game* os) {
    for (eapp_input_event* ev = os->input; ev; ev = ev->next)
        if (ev->button == EAPP_BTN_MENU && ev->action == EAPP_ACT_UP)
            quit = 1;
    eapp_gl2d_begin();
    eapp_gl2d_rect(0, 0, EAPP_SCREEN_W, EAPP_SCREEN_H, 0xC00000);
    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
}

void eapp_frame(eapp_os_to_game* os, eapp_game_to_os* game) {
    if (os->command != EAPP_CMD_RUN || quit) {
        game->state = EAPP_STATE_DONE;
        return;
    }
    if (failed) {
        show_error(os);
        return;
    }
    int status = setjmp(ipod_exit_jmp);
    if (status) {
        // Doom called exit(): 0 from Quit Game, -1 from I_Error. Its state is gone.
        if (status < 0) {
            failed = 1;
            ipod_log_save();
            show_error(os);
        } else {
            ipod_log_save();
            quit = 1;
            game->state = EAPP_STATE_DONE;
        }
        return;
    }

    handle_input(os);
    if (menu_held && DG_GetTicksMs() - menu_down_ms >= QUIT_HOLD_MS) {
        // Emergency exit: works even when Doom's screen or menu is unusable.
        printf("ipod: Menu held, quitting after %u frames\n", frames);
        ipod_log_save();
        quit = 1;
        game->state = EAPP_STATE_DONE;
        return;
    }
    frames++;

    if (!started) {
        started = 1;
        doomgeneric_Create(sizeof(doom_argv) / sizeof(doom_argv[0]) - 1, doom_argv);
    } else {
        doomgeneric_Tick();
    }

    present(os);
}
