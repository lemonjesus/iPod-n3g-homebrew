//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Ported to iPod via LLM
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// DESCRIPTION:
//	Zone memory allocation for the iPod: doomgeneric's z_zone.c spread over several
//	zones, because the OS heap only hands out blocks up to a fixed size. malloc and
//	friends are routed here too (as PU_STATIC blocks), so Doom and the C library share
//	one pool.
//

#include <stdio.h>
#include <string.h>

#include "eapp.h"
#include "z_zone.h"
#include "i_system.h"
#include "doomtype.h"

#define MEM_ALIGN 8
#define ZONEID	0x1d4a11
#define MINFRAGMENT 64

typedef struct memblock_s
{
    int			size;	// including the header and possibly tiny fragments
    void**		user;
    int			tag;	// PU_FREE if this is free
    int			id;	// should be ZONEID
    struct memblock_s*	next;
    struct memblock_s*	prev;
} memblock_t;

typedef struct
{
    int		size;       // total bytes, including this header
    memblock_t	blocklist;  // start / end cap for linked list
    memblock_t*	rover;
} memzone_t;

#define MAX_ZONES 32

static memzone_t *zones[MAX_ZONES];
static int num_zones;
static int last_zone;   // where the last allocation succeeded
static uint32_t os_block_size;

static void AddZone(void *mem, int size)
{
    memzone_t *zone = mem;
    memblock_t *block;

    if (num_zones == MAX_ZONES)
        return;

    zone->size = size;
    zone->blocklist.next = zone->blocklist.prev = block =
        (memblock_t *)((byte *)zone + sizeof(memzone_t));
    zone->blocklist.user = (void *)zone;
    zone->blocklist.tag = PU_STATIC;
    zone->rover = block;

    block->prev = block->next = &zone->blocklist;
    block->tag = PU_FREE;
    block->user = NULL;
    block->size = zone->size - sizeof(memzone_t);

    zones[num_zones++] = zone;
}

// Grab the OS heap. MemoryAlloc refuses anything bigger than its internal block size,
// so find that size, take as many blocks of it as the OS gives, then scoop up large
// leftovers. The last few hundred KB stay with the OS.
#define PROBE_MAX   (4 * 1024 * 1024)
#define PROBE_STEP  (16 * 1024)
#define SCOOP_MIN   (256 * 1024)

static void ipod_mem_init(void)
{
    uint32_t size;
    void *p = NULL;

    for (size = PROBE_MAX; size >= PROBE_STEP; size -= PROBE_STEP)
        if ((p = eapp_malloc(size)) != NULL)
            break;
    if (!p)
        return;

    os_block_size = size;
    AddZone(p, size);
    while (num_zones < MAX_ZONES && (p = eapp_malloc(size)) != NULL)
        AddZone(p, size);

    for (size /= 2; size >= SCOOP_MIN; size /= 2)
        while (num_zones < MAX_ZONES && (p = eapp_malloc(size)) != NULL)
            AddZone(p, size);
}

static memzone_t *ZoneOf(void *ptr)
{
    int i;

    for (i = 0; i < num_zones; i++)
        if ((byte *)ptr > (byte *)zones[i] && (byte *)ptr < (byte *)zones[i] + zones[i]->size)
            return zones[i];
    I_Error("Z: pointer %p is in no zone", ptr);
    return NULL;
}

void Z_Init (void)
{
    int i;
    unsigned total = 0;

    if (num_zones == 0)
        ipod_mem_init();
    if (num_zones == 0)
        I_Error("Z_Init: no memory from the OS heap");

    for (i = 0; i < num_zones; i++)
        total += zones[i]->size;
    printf("Z_Init: OS block %u bytes, %d zones, %u bytes total\n",
           (unsigned)os_block_size, num_zones, total);
    for (i = 0; i < num_zones; i++)
        printf("  zone %d: %p + %d\n", i, (void *)zones[i], zones[i]->size);
}

void Z_Free (void* ptr)
{
    memblock_t*		block;
    memblock_t*		other;
    memzone_t*		zone;

    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
	I_Error ("Z_Free: freed a pointer without ZONEID");

    zone = ZoneOf(block);

    if (block->tag != PU_FREE && block->user != NULL)
    {
    	// clear the user's mark
	    *block->user = 0;
    }

    // mark as free
    block->tag = PU_FREE;
    block->user = NULL;
    block->id = 0;

    other = block->prev;

    if (other->tag == PU_FREE)
    {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;

        if (block == zone->rover)
            zone->rover = other;

        block = other;
    }

    other = block->next;
    if (other->tag == PU_FREE)
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;

        if (other == zone->rover)
            zone->rover = block;
    }
}

// The original Z_Malloc for one zone, returning NULL instead of failing.
static void *ZoneMalloc(memzone_t *zone, int size, int tag, void *user)
{
    int		extra;
    memblock_t*	start;
    memblock_t* rover;
    memblock_t* newblock;
    memblock_t*	base;
    void *result;

    if (size > zone->size)
        return NULL;

    // if there is a free block behind the rover,
    //  back up over them
    base = zone->rover;

    if (base->prev->tag == PU_FREE)
        base = base->prev;

    rover = base;
    start = base->prev;

    do
    {
        if (rover == start)
        {
            // scanned all the way around the list
            return NULL;
        }

        if (rover->tag != PU_FREE)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                // so move base past it
                base = rover = rover->next;
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base = base->prev;
                Z_Free ((byte *)rover+sizeof(memblock_t));
                base = base->next;
                rover = base->next;
            }
        }
        else
        {
            rover = rover->next;
        }

    } while (base->tag != PU_FREE || base->size < size);

    // found a block big enough
    extra = base->size - size;

    if (extra >  MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        newblock = (memblock_t *) ((byte *)base + size );
        newblock->size = extra;

        newblock->tag = PU_FREE;
        newblock->user = NULL;
        newblock->prev = base;
        newblock->next = base->next;
        newblock->next->prev = newblock;

        base->next = newblock;
        base->size = size;
    }

    base->user = user;
    base->tag = tag;

    result  = (void *) ((byte *)base + sizeof(memblock_t));

    if (base->user)
    {
        *base->user = result;
    }

    // next allocation will start looking here
    zone->rover = base->next;

    base->id = ZONEID;

    return result;
}

void *Z_Malloc(int size, int tag, void *user)
{
    int i;
    void *result;

    if (user == NULL && tag >= PU_PURGELEVEL)
        I_Error ("Z_Malloc: an owner is required for purgable blocks");

    if (num_zones == 0)
        Z_Init();

    size = (size + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);
    size += sizeof(memblock_t);

    // Start with the zone that last had room, then try the others.
    for (i = 0; i < num_zones; i++)
    {
        int z = (last_zone + i) % num_zones;

        result = ZoneMalloc(zones[z], size, tag, user);
        if (result != NULL)
        {
            last_zone = z;
            return result;
        }
    }

    I_Error ("Z_Malloc: failed on allocation of %i bytes (%d zones of up to %u)",
             size, num_zones, (unsigned)os_block_size);
    return NULL;
}

void Z_FreeTags(int lowtag, int hightag)
{
    int i;
    memblock_t*	block;
    memblock_t*	next;

    for (i = 0; i < num_zones; i++)
    {
        for (block = zones[i]->blocklist.next ;
             block != &zones[i]->blocklist ;
             block = next)
        {
            // get link before freeing
            next = block->next;

            // free block?
            if (block->tag == PU_FREE)
                continue;

            if (block->tag >= lowtag && block->tag <= hightag)
                Z_Free ( (byte *)block+sizeof(memblock_t));
        }
    }
}

void Z_DumpHeap(int lowtag, int hightag)
{
    (void)lowtag;
    (void)hightag;
}

void Z_FileDumpHeap(FILE *f)
{
    (void)f;
}

void Z_CheckHeap (void)
{
    int i;
    memblock_t*	block;

    for (i = 0; i < num_zones; i++)
    {
        for (block = zones[i]->blocklist.next ; ; block = block->next)
        {
            if (block->next == &zones[i]->blocklist)
            {
                // all blocks have been hit
                break;
            }

            if ( (byte *)block + block->size != (byte *)block->next)
                I_Error ("Z_CheckHeap: block size does not touch the next block\n");

            if ( block->next->prev != block)
                I_Error ("Z_CheckHeap: next block doesn't have proper back link\n");

            if (block->tag == PU_FREE && block->next->tag == PU_FREE)
                I_Error ("Z_CheckHeap: two consecutive free blocks\n");
        }
    }
}

void Z_ChangeTag2(void *ptr, int tag, char *file, int line)
{
    memblock_t*	block;

    block = (memblock_t *) ((byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error("%s:%i: Z_ChangeTag: block without a ZONEID!",
                file, line);

    if (tag >= PU_PURGELEVEL && block->user == NULL)
        I_Error("%s:%i: Z_ChangeTag: an owner is required "
                "for purgable blocks", file, line);

    block->tag = tag;
}

void Z_ChangeUser(void *ptr, void **user)
{
    memblock_t*	block;

    block = (memblock_t *) ((byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
    {
        I_Error("Z_ChangeUser: Tried to change user for invalid block!");
    }

    block->user = user;
    *user = ptr;
}

int Z_FreeMemory (void)
{
    int i;
    memblock_t*		block;
    int			free;

    free = 0;

    for (i = 0; i < num_zones; i++)
    {
        for (block = zones[i]->blocklist.next ;
             block != &zones[i]->blocklist;
             block = block->next)
        {
            if (block->tag == PU_FREE || block->tag >= PU_PURGELEVEL)
                free += block->size;
        }
    }

    return free;
}

unsigned int Z_ZoneSize(void)
{
    int i;
    unsigned int total = 0;

    for (i = 0; i < num_zones; i++)
        total += zones[i]->size;
    return total;
}

// ---- C library allocator ------------------------------------------------------
// Permanent (PU_STATIC) zone blocks. newlib's stdio calls the _r versions.

struct _reent;

void *malloc(size_t n)
{
    return Z_Malloc(n ? (int)n : 1, PU_STATIC, NULL);
}

void free(void *p)
{
    if (p)
        Z_Free(p);
}

void *calloc(size_t count, size_t n)
{
    void *p = malloc(count * n);

    memset(p, 0, count * n);
    return p;
}

void *realloc(void *p, size_t n)
{
    memblock_t *block;
    size_t old;
    void *q;

    if (!p)
        return malloc(n);
    if (!n)
    {
        free(p);
        return NULL;
    }
    block = (memblock_t *)((byte *)p - sizeof(memblock_t));
    old = block->size - sizeof(memblock_t);
    if (n <= old)
        return p;
    q = malloc(n);
    memcpy(q, p, old);
    free(p);
    return q;
}

void *_malloc_r(struct _reent *r, size_t n) { (void)r; return malloc(n); }
void _free_r(struct _reent *r, void *p) { (void)r; free(p); }
void *_calloc_r(struct _reent *r, size_t c, size_t n) { (void)r; return calloc(c, n); }
void *_realloc_r(struct _reent *r, void *p, size_t n) { (void)r; return realloc(p, n); }
