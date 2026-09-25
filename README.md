# iPod nano 3G homebrew SDK

Write your own click-wheel games ("eApps") for the iPod nano 3rd generation in C. The SDK builds a game folder you copy onto the iPod; it then shows up under **Games** like any other game.

## You need

- An iPod nano 3G running an OS with the game signature check patched out (see below). Stock firmware refuses every homebrew game.
- The GNU ARM toolchain: `arm-none-eabi-gcc`, `arm-none-eabi-ld`, `arm-none-eabi-objcopy` (`binutils-arm-none-eabi` + `gcc-arm-none-eabi` on Linux, `arm-none-eabi-gcc` via Homebrew on macOS). `arm-none-eabi-gcc` also supplies libgcc (runtime division, 64-bit maths, floats).
- Python 3.

macOS (Homebrew):

```bash
brew install arm-none-eabi-gcc python
```

Debian/Ubuntu:

```bash
sudo apt install binutils-arm-none-eabi gcc-arm-none-eabi python3
```

If your toolchain uses a different prefix or lives outside `PATH`, set `CC`, `LD`,
`OBJCOPY`, `LIBGCC` directly, e.g. `make CC=/path/to/arm-none-eabi-gcc`.

## Quick start

```bash
cd examples
cp -r hello mygame
cd mygame
# edit Makefile: NAME (menu title), FOLDER (unique id); EAPP_SDK must point at this repository
make
make install IPOD=<path to plugged in iPod>
```

Eject the iPod, open **Games**, pick your game.

## The Patch (required)

At `0x000FA608` in `osos.bin`, change `05 30 A0 E1` (`mov r3, r5`) to `00 30 A0 E3` (`mov r3, #0`). That passes a NULL signature path to the plist loader, and thus skips the signature verification. How to patch and flash `osos` is out of scope for this repository - see the [16GB iPod Nano 3rd Generation](https://github.com/lemonjesus/iPod-n3g-16gb) project for how to use wInd3x to accomplish this.

Make sure the patched build is what is actually flashed and running. An unpatched OS fails every homebrew game with "This game cannot be launched.", which looks exactly like a broken game.

**Very important to note:** that bypassing this signature check *does not* allow you to run officially published games you do not have the rights to.

## What works

| Area | Status |
|---|---|
| Loading, linking, launching, exiting back to the menu | Confirmed on hardware |
| Buttons (Menu, Select, Prev, Next, Play) and click wheel | Confirmed |
| Drawing with OpenGL ES 1.x (software renderer) | Confirmed; two quirks, see [Drawing](#drawing) |
| Heap allocation (`MemoryAlloc` module) | Declared, untested |
| Volume, platform id (`miscTBD` module) | Declared, untested |
| Sound, file IO, save data, settings, launch artwork | Not mapped yet |

## Writing a game

A game is three C functions, all optional except `eapp_frame`:

```c
#include "eapp.h"

void eapp_init(void);                                         // once, after loading
void eapp_frame(eapp_os_to_game *os, eapp_game_to_os *game);  // every frame
void eapp_shutdown(void);                                     // once, when unloaded
```

The OS:

1. Loads your compiled game, fills in the OS function pointers, then calls `eapp_init()`.
2. Creates a 320x240 EGL window surface and context and makes them current.
3. Calls `eapp_frame(os, game)` repeatedly, about every 33 to 102 ms (the OS adapts the period; there is no vsync control). Each call: read input, update, draw, then `eglSwapBuffers()`. Nothing reaches the screen without the swap.
4. When the game sets `game->state = EAPP_STATE_DONE`, the loop ends and the iPod returns to the Games menu.

A minimal game:

```c
#include "eapp.h"
#include "eapp_gles.h"
#include "eapp_2d.h"

void eapp_frame(eapp_os_to_game *os, eapp_game_to_os *game) {
    if (os->command != EAPP_CMD_RUN) {
        game->state = EAPP_STATE_DONE;
        return;
    }
    for (eapp_input_event *ev = os->input; ev; ev = ev->next)
        if (ev->button == EAPP_BTN_MENU && ev->action == EAPP_ACT_UP)
            game->state = EAPP_STATE_DONE;

    eapp_gl2d_begin();
    eapp_gl2d_rect(0, 0, EAPP_SCREEN_W, EAPP_SCREEN_H, 0x000080);
    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
}
```

`examples/hello/hello.c` is a slightly larger version with the wheel and all buttons.

### Exiting

Set `game->state = EAPP_STATE_DONE` from `eapp_frame`. The iPod goes back to the Games menu. Always honour `os->command != EAPP_CMD_RUN` the same way: the OS sets it to `EAPP_CMD_QUIT` (5) or `EAPP_CMD_STOP_OTHER` (4) when it needs the game to stop.

Give players a way out. Menu is the natural quit (or pause-menu) button.

### Input

`os->input` is a linked list of button events since the previous frame (the OS frees it; don't keep pointers across frames):

```c
typedef struct eapp_input_event {
    uint8_t  button;    // EAPP_BTN_*
    uint8_t  action;    // EAPP_ACT_DOWN (2) or EAPP_ACT_UP (1)
    uint16_t pad;
    uint32_t time_ms;   // timestamp
    struct eapp_input_event *next;
} eapp_input_event;
```

| Constant | Id | Physical button |
|---|---|---|
| `EAPP_BTN_MENU` | 1 | Menu (top of the wheel) |
| `EAPP_BTN_SELECT` | 2 | center button |
| `EAPP_BTN_NEXT` | 3 | Next (right) |
| `EAPP_BTN_PREV` | 4 | Previous (left) |
| `EAPP_BTN_PLAY` | 5 | Play/Pause (bottom) |
| `EAPP_BTN_OTHER` | 0 | I'm not sure if/how this is used |

To track held buttons, set a flag on `EAPP_ACT_DOWN` and clear it on `EAPP_ACT_UP`.

`eapp_pass_event_to_os(ev)` hands an event to the OS's default handler, which acts on releases of Menu, Next, Prev and Play, ending the game on a Menu release. *Untested* - quitting via `game->state` is the proven path.

Click wheel:

```c
int delta;
uint32_t extra;
eapp_read_wheel(&delta, &extra);
```

`delta` is the wheel movement since the previous call; clockwise is positive. The OS resets its counter on each call, so call it once per frame. `extra` is a second value from the wheel driver whose meaning is not known yet.

### Drawing

The screen is 320x240 (`EAPP_SCREEN_W`, `EAPP_SCREEN_H`), 16-bit RGB565, double buffered. The `OpenGLES` import provides OpenGL ES 1.x and EGL 1.1 (full list in `sdk/include/eapp_gles.h`). The renderer is software. Two differences from desktop GL:

1. **Row 0 is the top.** GL window y=0 is the top row of the screen. For pixel coordinates with (0,0) at the top-left, use `glOrthox(0, 320<<16, 0, 240<<16, -1<<16, 1<<16)`, which is what `eapp_gl2d_begin()` does. The usual desktop-GL projection (bottom=240, top=0) draws everything upside down.
2. **`glClear` ignores the scissor box.** With the default colour mask, a scissored `glClear` clears the whole screen. Clear the full screen only, and draw rectangles as quads (`eapp_gl2d_rect()`).

Other things to be aware of:

- Use the fixed-point (`x`) functions: 16.16 fixed point, `GL_FIXED_ONE` = 1.0. There is no FPU, so every float call goes through soft-float helpers.
- Two texture units. `glCompressedTexImage2D` accepts only paletted formats (`GL_PALETTE*_OES`).
- Not exported: `glColor4ub`, `glTexEnvi/iv`, `glDrawTex*OES`, `glPointSizePointerOES`, matrix palette, `eglGetProcAddress`.
- A depth buffer exists (the OS creates the surface with one).
- The EGL display, surface and context already exist and are current. Don't create your own.
- `glFinish`/`glFlush` do nothing; `eglSwapBuffers()` presents the frame.

The helpers in `sdk/include/eapp_2d.h`:

| Function | Does |
|---|---|
| `eapp_gl2d_begin()` | Viewport, y-down pixel projection, texturing/depth/blend off, vertex arrays on. Call once per frame before `eapp_gl2d_rect`. |
| `eapp_gl2d_rect(x, y, w, h, 0xRRGGBB)` | Filled rectangle as a GL quad |

### Memory

| Region | Size | Notes |
|---|---|---|
| Program + globals | 1 MB total | Code, constants, initialised data and zeroed globals (`.bss`) share one fixed 1 MB window. The link fails if you exceed it. |
| Stack | about 14 KB | Shared with OS code. Make big arrays `static` or global, never local. |
| Heap | `HEAP_KB` (default 1024, max 5120) | `eapp_malloc`/`eapp_free` from the `MemoryAlloc` import. |

Globals start zeroed (the SDK's startup code clears `.bss` before `eapp_init`). Initialised globals keep their values, because they are part of `Game.bin`. The program is never relocated, so pointers to globals are fixed.

### C environment

- Freestanding C11 compiled for ARMv5TE in ARM mode, soft-float ABI. `stdint.h`, `stddef.h`, `stdbool.h`, `stdarg.h` and `limits.h` all work (they come with the compiler).
- No C library. The SDK provides `memset`, `memcpy` and `memmove` only: no `printf`, `malloc`, `strlen`, `rand` or `<math.h>`. Write small replacements as needed.
- libgcc, when found, supplies runtime integer division and modulo, 64-bit arithmetic and float/double arithmetic. Without it those operations fail to link (division by a constant still works; the compiler turns it into a multiply).
- Floats work but are slow: prefer integers and fixed point.
- No C++ runtime (no exceptions, RTTI or global constructors).
- `-ffunction-sections` + `--gc-sections`: unused functions are dropped

### OS modules

The OS exports functions in named modules. `IMPORTS` in your Makefile lists the modules a game links against; each one adds a small stub table. Functions are called by name like any C function.

| Module | Functions | SDK status |
|---|---|---|
| `OpenGLES` | 170 | All named and prototyped in `eapp_gles.h`; drawing confirmed |
| `InputEvents` | 2 | `eapp_read_wheel` (confirmed), `eapp_pass_event_to_os` (untested) |
| `MemoryAlloc` | 3 | `eapp_malloc` (slot 0), `eapp_free` (slot 2) |
| `miscTBD` | 13 | `misc_SetVolume`, `misc_GetVolume`, `misc_GetPlatformID` |
| `Settings` | 1 | `eapp_get_setting`, no prototype yet |
| `AsyncFileIO` | 17 | Unmapped |
| `SoundEffect` | 42 | Unmapped |
| `Audio` | 54 | Unmapped |
| `Metadata` | 169 | Unmapped (music library access) |
| `Users` | 10 | Unmapped |
| `DebugUtil` | 5 | Unmapped |

An unmapped function can still be imported and called by its placeholder name, but without a known signature that isn't game code you can write against yet. The module table (names, UUIDs, function order) is `tools/modules.py`; the OS matches modules by name, UUID and exact function count, so don't edit it by hand.

`Game.bin` is the only file the SDK installs. Put images, fonts, levels and similar data in C arrays compiled into the program (`static const uint16_t sprite[] = {...}`). File access from a game (`AsyncFileIO`) is not mapped yet.

### Performance

The CPU has no FPU, and GL runs in software, so some things to keep in mind:

- Full-screen redraws are fine at the OS's frame rate; avoid per-pixel floating point.
- Fixed point (16.16) for positions and velocities.
- Precompute tables (sine, colours) as `const` arrays.
- `eglSwapBuffers` alternates between two buffers, so redrawing only a changed region would need to reach both of them (two frames) to stick. Redrawing everything each frame is simpler and avoids that bookkeeping.

### Reference: blocks passed to `eapp_frame`

`eapp_os_to_game` (OS to game, 256 bytes):

| Field | Offset | Meaning |
|---|---|---|
| `command` | 0x00 | `EAPP_CMD_RUN` (0), `EAPP_CMD_STOP_OTHER` (4), `EAPP_CMD_QUIT` (5) |
| `async_done` | 0x2C | Finished async file requests (AsyncFileIO, unmapped) |
| `input` | 0x30 | Button events since the last frame |

`eapp_game_to_os` (game to OS, 248 bytes):

| Field | Offset | Meaning |
|---|---|---|
| `state` | 0x00 | `EAPP_STATE_RUNNING` (0), `EAPP_STATE_EXITING` (5), `EAPP_STATE_DONE` (6) |
| `exit_arg` | 0x24 | Passed on with exit reason 5 (purpose not documented) |
| `exit_req_5/6/4` | 0x28-0x2A | Exit-reason flags read at shutdown (purpose not documented) |

Leave the other bytes alone.

## Building, installing, and creating your own game

Copy the starter game and point it at this repository:

```bash
cp -r examples/hello ~/mygame
cd ~/mygame
```

Edit `Makefile`:

```make
EAPP_SDK := /path/to/this/repository
NAME     := My Game              # title in the Games menu
FOLDER   := 4D5947414D453031     # unique id; also the folder name on the iPod
SRCS     := hello.c              # add more .c files here
include $(EAPP_SDK)/eapp.mk
```

`FOLDER` rules: letters and digits only, at most 16 characters, different for every game on the iPod. It doubles as the manifest `GUID` (retail games do the same). Changing it later installs a second copy rather than replacing the first.

Other settings you can put before the `include`:

| Variable | Default | Meaning |
|---|---|---|
| `IMPORTS` | `OpenGLES InputEvents` | OS modules to link against (see [OS modules](#os-modules)) |
| `VERSION` | `1.0` | Shown in the game's info |
| `HEAP_KB` | `1024` | Heap requested from the OS, in KB (max 5120) |
| `EXE` | `Game.bin` | Executable file name inside the game folder |
| `EXTRA_CFLAGS` | empty | Extra compiler flags, e.g. `-DDEBUG` |
| `B` | `build/$(FOLDER)` | Build directory |

Build:

```bash
make
```

The game is built to `build/<FOLDER>/<FOLDER>/`: `Game.bin` and `Manifest.plist`.

To install the game, plug your iPod in such that you can see it with a file browser, then:

```bash
make install IPOD=/Volumes/YOURIPOD        # macOS
make install IPOD=/media/$USER/IPOD        # Linux
```

This copies `Manifest.plist` and `Game.bin` to `iPod_Control/Games_RO/<FOLDER>/` (whichever capitalisation already exists; FAT is case-insensitive). Eject the iPod properly, then open **Games**. The game is listed under its `NAME`.

To remove a game, delete its folder from `iPod_Control/Games_RO/`.

## Disclosure of AI Use
Some parts of this project were researched and developed with the aid of an LLM. Files written completely by an LLM are marked as such. Every line of code was reviewed and tested by me regardless of who or what wrote it.
