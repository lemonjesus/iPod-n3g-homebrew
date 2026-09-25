# Doom for the iPod nano 3G, via doomgeneric (git submodule in ./doomgeneric).
#   git submodule update --init   fetch doomgeneric first
#   make                          build
#   make install IPOD=/Volumes/MyiPod   also copies doom1.wad (put it in this folder)
# written by an LLM

EAPP_SDK := ../..
NAME     := Doom
FOLDER   := 444F4F4D47454E31

DG       := doomgeneric/doomgeneric

# The engine sources from doomgeneric's own Makefile, minus its platform files and
# z_zone (replaced by z_ipod.c).
DOOM     := dummy am_map doomdef doomstat dstrings d_event d_items d_iwad d_loop \
            d_main d_mode d_net f_finale f_wipe g_game hu_lib hu_stuff info i_cdmus \
            i_endoom i_joystick i_scale i_sound i_system i_timer memio m_argv m_bbox \
            m_cheat m_config m_controls m_fixed m_menu m_misc m_random p_ceilng p_doors \
            p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj p_plats p_pspr p_saveg \
            p_setup p_sight p_spec p_switch p_telept p_tick p_user r_bsp r_data r_draw \
            r_main r_plane r_segs r_sky r_things sha1 sounds statdump st_lib st_stuff \
            s_sound tables v_video wi_stuff w_checksum w_file w_main w_wad \
            w_file_stdc i_input i_video doomgeneric

SRCS     := doomgeneric_ipod.c ipod_syscalls.c z_ipod.c $(addprefix $(DG)/,$(addsuffix .c,$(DOOM)))

IMPORTS  := OpenGLES InputEvents MemoryAlloc miscTBD DebugUtil
HEAP_KB  := 5120

# Doom draws RGB565 straight into a buffer that doubles as a 512x256 GL texture
# (see doomgeneric_ipod.c for why RESY is 200).
EXTRA_CFLAGS := -I$(DG) -DDOOMGENERIC_RESX=512 -DDOOMGENERIC_RESY=200 -D_DEFAULT_SOURCE

# Doom needs a real C library (stdio, strings, malloc): newlib-nano, which ships
# with arm-none-eabi-gcc. ipod_syscalls.c supplies the OS glue it calls into, and
# z_ipod.c its malloc.
NEWLIB   = $(shell $(CC) $(TARGET) -print-file-name=$(1))
LDLIBS    = $(call NEWLIB,libc_nano.a) $(call NEWLIB,libm.a)

# The shareware doom1.wad is installed next to Game.bin; Doom reads it from there.
INSTALL_FILES := doom1.wad

include $(EAPP_SDK)/eapp.mk

# Keep the SDK's warnings for our files, silence them for the upstream engine.
$(B)/obj/$(DG)/%.o: CFLAGS += -w
