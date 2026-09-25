/*
 * eapp.h - iPod nano 3G "eApp" (click-wheel game) runtime interface.
 *
 * Execution model:
 *   - The image is read into a fixed 1 MB window and never relocated.
 *   - eapp_init() runs once after load.
 *   - eapp_frame(os, game) runs once per frame. An EGL context and window
 *     surface are already current. Call eglSwapBuffers() to put a frame on screen.
 *   - eapp_shutdown() runs when the game is unloaded.
 *   - ARM (not Thumb), soft-float ABI.
 */
#ifndef EAPP_H
#define EAPP_H
#include <stdint.h>

#define EAPP_SCREEN_W 320
#define EAPP_SCREEN_H 240

typedef struct eapp_input_event {
    uint8_t  button;   // 1-5
    uint8_t  action;   // EAPP_ACT_DOWN / EAPP_ACT_UP
    uint16_t pad;
    uint32_t time_ms;
    struct eapp_input_event* next;
} eapp_input_event;

enum { EAPP_ACT_UP = 1, EAPP_ACT_DOWN = 2 };

// Button UDs
enum {
    EAPP_BTN_OTHER  = 0,   // Not sure what this is or how it gets produced
    EAPP_BTN_MENU   = 1,
    EAPP_BTN_SELECT = 2,
    EAPP_BTN_NEXT   = 3,
    EAPP_BTN_PREV   = 4,
    EAPP_BTN_PLAY   = 5,
};

// OS -> game block: first argument of eapp_frame (0x100 bytes).
typedef struct eapp_os_to_game {
    uint8_t  command;              // 0 = run, 5 = OS wants you to quit, 4 = other stop
    uint8_t  _r0[0x2C - 0x01];
    void* async_done;           // AsyncFileIO requests completed since last frame
    eapp_input_event* input;       // button events since last frame (freed by the OS)
    uint8_t  _r1[0x100 - 0x34];    // the EGL back buffer pointer - use GL to draw instead
} eapp_os_to_game;

enum { EAPP_CMD_RUN = 0, EAPP_CMD_STOP_OTHER = 4, EAPP_CMD_QUIT = 5 };

// Game -> OS block: second argument of eapp_frame (0xF8 bytes).
typedef struct eapp_game_to_os {
    uint8_t  state;                // set EAPP_STATE_DONE to leave
    uint8_t  _r0[0x24 - 0x01];
    uint32_t exit_arg;             // passed on with exit reason 5
    uint8_t  exit_req_5;
    uint8_t  exit_req_6;
    uint8_t  exit_req_4;
    uint8_t  _r1[0xF8 - 0x2B];
} eapp_game_to_os;

// 5 = exiting (OS keeps calling eapp_frame for several frames), 6 = done.
enum { EAPP_STATE_RUNNING = 0, EAPP_STATE_EXITING = 5, EAPP_STATE_DONE = 6 };

// ---- Game entry points (implement these) ----
void eapp_init(void);                                         // optional
void eapp_frame(eapp_os_to_game* os, eapp_game_to_os* game);  // required
void eapp_shutdown(void);                                     // optional

// Wheel movement since the previous call (OS resets its counter); clockwise is
// positive (confirmed on hardware).
void eapp_read_wheel(int* delta, uint32_t* extra);

// Hand an event to the OS default handler (acts on releases of buttons 1,3,4,5).
// For example, if the user hits Play/Pause and you want the music to respond accordingly
void eapp_pass_event_to_os(const eapp_input_event* ev);

// MemoryAlloc (heap sized by Manifest HeapSize)
void* eapp_malloc(uint32_t size);
void eapp_free(void* p, uint32_t unknown);

// miscTBD module (partially identified)
int misc_GetPlatformID(void);
int misc_GetVolume(void);
int misc_SetVolume(int percent_0_100);

#endif
