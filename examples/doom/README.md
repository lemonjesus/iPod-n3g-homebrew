# Doom (work in progress)

A port of [doomgeneric](https://github.com/ozkl/doomgeneric) to the eApp SDK.

```bash
git submodule update --init   # fetch doomgeneric
cp /path/to/doom1.wad .       # shareware v1.9, md5 f0cefca49926d00903cf57551d901abe
make
make install IPOD=/media/$USER/IPOD   # copies Game.bin, Manifest.plist and doom1.wad
```

If Doom stops with an error, the screen turns red until you press Menu, and it tries to save
its console output to `iPod_Control/gamedata_RW/<FOLDER>/doom.log`.

## Files

| File | What it does |
|---|---|
| `doomgeneric_ipod.c` | The doomgeneric platform functions (`DG_*`), input, drawing, and `eapp_frame` |
| `ipod_syscalls.c` | The functions newlib-nano calls into the OS for: `exit`, files, console output |
| `z_ipod.c` | Doom's zone allocator (from `z_zone.c`) spread over several OS heap blocks, plus `malloc` |
| `doomgeneric/` | Upstream engine, a git submodule. It is not modified. |

## How it fits the SDK

- **Main loop.** doomgeneric expects to own `main()`. Instead, the first `eapp_frame` runs `doomgeneric_Create` (all of Doom's startup) and every frame after that runs one `doomgeneric_Tick`.
- **C library.** Doom uses stdio, string functions and `malloc`, so it links newlib-nano from the ARM toolchain (through the new `LDLIBS` setting in `eapp.mk`). `_sbrk` takes one big block from `eapp_malloc`.
- **Exit.** `exit()` and `I_Error` `longjmp` back into `eapp_frame`, which sets `EAPP_STATE_DONE`.
- **Video.** Doom runs with `-gfxmode rgb565`, so it writes screen-ready pixels (rows 512 pixels apart). Each frame copies the 320x200 picture straight into the OS back buffer (`os->backbuffer`), centred on the 320x240 screen, and calls `eglSwapBuffers`. (A GL texture upload showed only noise on hardware.)
- **Files.** `_open`/`_read`/`_lseek` use the synchronous `DebugUtil` file calls (`eapp_file_*`), read only, from the game's own folder. Writes (config, saves) are refused for now.
- **Time.** `DG_GetTicksMs` uses the OS's microsecond counter (`misc_GetUsecTimer`). Doom only sleeps while waiting for its next tic, so `DG_SleepMs` busy-waits.
- **Memory.** Image 355 KB + `.bss` 244 KB = about 600 KB of the 1 MB program window. The OS heap refuses single allocations over its 512 KB block size, so `z_ipod.c` runs Doom's zone over every block it can get (on hardware: 2 x 512 KB + 14 x 256 KB = 4.5 MB), and `malloc` shares it. The largest stack frame is about 550 bytes, well within the ~14 KB stack.

## Controls

| Button | In game | In the menu |
|---|---|---|
| Wheel | turn | up/down |
| Select | fire | choose |
| Play | move forward | "yes" to prompts |
| Next | use (doors, switches) | slider right |
| Prev | move back | slider left |
| Menu | open menu | back |
| Hold Menu 2 s | quit (always works, saves `doom.log`) | quit |

No strafing, run or weapon keys yet. Doom switches weapons on pickup.
