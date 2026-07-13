/*
 * gfxcore.c - screen/window/tile plumbing for the graphical renderer
 *
 * OS-friendly custom screen: Intuition owns the copper list and display
 * DMA, multitasking stays alive so serial I/O to fujinet-nio keeps running
 * underneath. All rendering goes through the backdrop window's RastPort,
 * so graphics.library clips it via the layer — no manual bounds paranoia.
 *
 * Blitter/sprite source data MUST live in chip RAM: the data hunk can land
 * in slow RAM (FS-UAE's default A500), so everything from tiles.h is staged
 * into one AllocMem(MEMF_CHIP) bank at init. The .datachip linker section
 * does NOT work on this toolchain — see apps/pacmantests/c99-pac-man/README.md.
 *
 * Requires: intuition.library, graphics.library (V33 / KS 1.3)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#include <string.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <graphics/sprite.h>
#include <graphics/view.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "gfxcore.h"
#include "tiles.h"
#include "amiga_vars.h"

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct Window *gfx_window;

static struct Screen *_screen;

/* Cursor hardware sprite (sprite 2 — sprite 0 is the Intuition pointer). */
#define CURSOR_SPR_NUM 2
static struct SimpleSprite _cursor_spr;
static BYTE _cursor_spr_got = -1;      /* GetSprite result, -1 = none */

/* Chip-RAM staging bank: all tiles, the two cursor sprite images, and a
 * zeroed blank mouse-pointer sprite (posctl + 1 line + terminator). */
#define TILE_BYTES       (8 * 4 * sizeof(UWORD))       /* one 8x8x4 tile */
#define TILE_BANK_BYTES  (TILE_COUNT * TILE_BYTES)
#define CURSOR_SPR_BYTES (CURSOR_SPR_WORDS * sizeof(UWORD))
#define BLANK_PTR_WORDS  6
#define BLANK_PTR_BYTES  (BLANK_PTR_WORDS * sizeof(UWORD))
#define CHIP_BANK_BYTES  (TILE_BANK_BYTES + 2 * CURSOR_SPR_BYTES + \
                          BLANK_PTR_BYTES)
static UWORD *_chip_bank;
static UWORD *_chip_cursor_a;
static UWORD *_chip_cursor_b;
static UWORD *_chip_blank_ptr;

/* Mouse-aim handshake (see gfxcore.h). */
static uint8_t _aim_active;
static uint8_t _aim_players;
static uint8_t _aim_x;
static uint8_t _aim_y;
static uint8_t _pointer_blanked;

/* Spare bitmap for saveScreenBuffer/restoreScreenBuffer (in-game menu). */
#define SCREEN_W    320
#define SCREEN_H    200
#define SCREEN_D    4
#define RAS_BYTES   ((SCREEN_W / 8) * SCREEN_H)
static struct BitMap _save_bm;
static PLANEPTR _save_planes;          /* one chunk, SCREEN_D * RAS_BYTES */

/* Explicit topaz 8: the *system default* font can be the 60-column topaz
 * (10 px wide), which overflows the 8x8 cell grid and misaligns every
 * string against the tile layout. TOPAZ_EIGHTY is always in ROM. */
static struct TextAttr _topaz8 = {
    (STRPTR)"topaz.font", TOPAZ_EIGHTY, FS_NORMAL, FPF_ROMFONT
};

static struct NewScreen _new_screen = {
    0, 0, SCREEN_W, SCREEN_H, SCREEN_D,
    0, 1,
    0,                 /* ViewModes: lores */
    CUSTOMSCREEN,
    &_topaz8,          /* 8x8 font to match the cell grid */
    NULL,              /* no title — ShowTitle(FALSE) anyway */
    NULL, NULL
};

static struct NewWindow _new_window = {
    0, 0, SCREEN_W, SCREEN_H,
    0, 1,
    RAWKEY | VANILLAKEY | MOUSEBUTTONS,
    BACKDROP | BORDERLESS | ACTIVATE,
    NULL, NULL, NULL,
    NULL,              /* Screen — filled in at open */
    NULL,
    0, 0, 0, 0,
    CUSTOMSCREEN
};

/* Reusable Image: ImageData is repointed into the chip bank per call. */
static struct Image _tile_img = {
    0, 0, 8, 8, SCREEN_D, NULL, 0x0F, 0x00, NULL
};

void gfx_close(void)
{
    if (_cursor_spr_got >= 0) {
        FreeSprite(CURSOR_SPR_NUM);
        _cursor_spr_got = -1;
    }
    if (gfx_window) {
        CloseWindow(gfx_window);
        gfx_window = NULL;
    }
    if (_screen) {
        CloseScreen(_screen);
        _screen = NULL;
    }
    if (_save_planes) {
        FreeMem(_save_planes, (ULONG)SCREEN_D * RAS_BYTES);
        _save_planes = NULL;
    }
    if (_chip_bank) {
        FreeMem(_chip_bank, CHIP_BANK_BYTES);
        _chip_bank = NULL;
    }
    if (IntuitionBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }
    if (GfxBase) {
        CloseLibrary((struct Library *)GfxBase);
        GfxBase = NULL;
    }
}

uint8_t gfx_open(void)
{
    uint8_t i;
    UWORD *dst;

    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    if (!GfxBase)
        goto fail;
    IntuitionBase = (struct IntuitionBase *)
        OpenLibrary((CONST_STRPTR)"intuition.library", 0);
    if (!IntuitionBase)
        goto fail;

    /* Stage every tile and both cursor sprite images into chip RAM. */
    _chip_bank = (UWORD *)AllocMem(CHIP_BANK_BYTES, MEMF_CHIP);
    if (!_chip_bank)
        goto fail;
    dst = _chip_bank;
    for (i = 0; i < TILE_COUNT; i++) {
        CopyMem((APTR)tile_table[i], dst, TILE_BYTES);
        dst += TILE_BYTES / sizeof(UWORD);
    }
    _chip_cursor_a = dst;
    CopyMem((APTR)cursor_spr_a, _chip_cursor_a, CURSOR_SPR_BYTES);
    _chip_cursor_b = dst + CURSOR_SPR_WORDS;
    CopyMem((APTR)cursor_spr_b, _chip_cursor_b, CURSOR_SPR_BYTES);
    _chip_blank_ptr = _chip_cursor_b + CURSOR_SPR_WORDS;
    memset(_chip_blank_ptr, 0, BLANK_PTR_BYTES);   /* invisible pointer */

    /* Save/restore buffer: the blitter reads it back on restore, so it
     * must be chip RAM like everything else the blitter touches. */
    _save_planes = (PLANEPTR)AllocMem((ULONG)SCREEN_D * RAS_BYTES, MEMF_CHIP);
    if (!_save_planes)
        goto fail;
    InitBitMap(&_save_bm, SCREEN_D, SCREEN_W, SCREEN_H);
    for (i = 0; i < SCREEN_D; i++)
        _save_bm.Planes[i] = _save_planes + (ULONG)i * RAS_BYTES;

    _screen = OpenScreen(&_new_screen);
    if (!_screen)
        goto fail;
    ShowTitle(_screen, FALSE);
    LoadRGB4(&_screen->ViewPort, (UWORD *)tile_palette, 16);

    _new_window.Screen = _screen;
    gfx_window = OpenWindow(&_new_window);
    if (!gfx_window)
        goto fail;

    /* Attack cursor sprite. Sprites 2/3 take their colors from registers
     * 21-23. A failed GetSprite just means no cursor overlay — the game
     * stays playable, so don't treat it as fatal. */
    _cursor_spr.x = 0;
    _cursor_spr.y = 0;
    _cursor_spr.height = CURSOR_SPR_HEIGHT;
    _cursor_spr.num = 0;
    _cursor_spr_got = GetSprite(&_cursor_spr, CURSOR_SPR_NUM);
    if (_cursor_spr_got >= 0) {
        for (i = 0; i < 3; i++)
            SetRGB4(&_screen->ViewPort, 21 + i,
                    (cursor_spr_rgb[i] >> 8) & 0xF,
                    (cursor_spr_rgb[i] >> 4) & 0xF,
                    cursor_spr_rgb[i] & 0xF);
        ChangeSprite(&_screen->ViewPort, &_cursor_spr, (APTR)_chip_cursor_a);
        gfx_cursor_hide();
    }

    atexit(gfx_close);
    return 1;

fail:
    gfx_close();
    return 0;
}

void gfx_draw_tile(uint8_t cx, uint8_t cy, uint8_t tile)
{
    if (cx >= WIDTH || cy >= HEIGHT || tile >= TILE_COUNT || !gfx_window)
        return;
    _tile_img.ImageData = _chip_bank + (ULONG)tile * (TILE_BYTES / sizeof(UWORD));
    DrawImage(gfx_window->RPort, &_tile_img,
              (WORD)cx * CM_CELL_W, (WORD)cy * CM_CELL_H);
}

void gfx_text(uint8_t cx, uint8_t cy, const char *s, uint8_t pen, uint8_t bpen)
{
    struct RastPort *rp;
    WORD len;

    if (cy >= HEIGHT || !gfx_window || !s)
        return;
    len = (WORD)strlen(s);
    if (len <= 0)
        return;
    /* Clip to the row so a long string can't wrap into layer clipping
     * weirdness (the layer clips pixels; we clip characters). */
    if (cx >= WIDTH)
        return;
    if (cx + len > WIDTH)
        len = WIDTH - cx;

    rp = gfx_window->RPort;
    SetAPen(rp, pen);
    SetBPen(rp, bpen);
    SetDrMd(rp, JAM2);
    Move(rp, (WORD)cx * CM_CELL_W, (WORD)cy * CM_CELL_H + rp->TxBaseline);
    Text(rp, (CONST_STRPTR)s, (ULONG)len);
}

void gfx_fill(uint8_t cx, uint8_t cy, uint8_t w, uint8_t h, uint8_t pen)
{
    struct RastPort *rp;

    if (cx >= WIDTH || cy >= HEIGHT || !w || !h || !gfx_window)
        return;
    if (cx + w > WIDTH)
        w = WIDTH - cx;
    if (cy + h > HEIGHT)
        h = HEIGHT - cy;

    rp = gfx_window->RPort;
    SetAPen(rp, pen);
    SetDrMd(rp, JAM1);
    RectFill(rp, (WORD)cx * CM_CELL_W, (WORD)cy * CM_CELL_H,
             (WORD)(cx + w) * CM_CELL_W - 1, (WORD)(cy + h) * CM_CELL_H - 1);
}

void gfx_cursor_move(uint8_t cx, uint8_t cy, uint8_t alt)
{
    if (_cursor_spr_got < 0)
        return;
    ChangeSprite(&_screen->ViewPort, &_cursor_spr,
                 (APTR)(alt ? _chip_cursor_b : _chip_cursor_a));
    MoveSprite(&_screen->ViewPort, &_cursor_spr,
               (WORD)cx * CM_CELL_W, (WORD)cy * CM_CELL_H);
}

void gfx_cursor_hide(void)
{
    if (_cursor_spr_got < 0)
        return;
    MoveSprite(&_screen->ViewPort, &_cursor_spr, 0, SCREEN_H + 16);
}

uint8_t gfx_save_screen(void)
{
    if (!_screen || !_save_planes)
        return 0;
    BltBitMap(&_screen->BitMap, 0, 0, &_save_bm, 0, 0,
              SCREEN_W, SCREEN_H, 0xC0, 0xFF, NULL);
    return 1;
}

void gfx_restore_screen(void)
{
    if (!_screen || !_save_planes)
        return;
    BltBitMap(&_save_bm, 0, 0, &_screen->BitMap, 0, 0,
              SCREEN_W, SCREEN_H, 0xC0, 0xFF, NULL);
}

void gfx_aim_set(uint8_t player_count, uint8_t cell_x, uint8_t cell_y)
{
    _aim_active = 1;
    _aim_players = player_count;
    _aim_x = cell_x;
    _aim_y = cell_y;
}

void gfx_aim_clear(void)
{
    _aim_active = 0;
    gfx_pointer_blank(0);
}

uint8_t gfx_aim_get(uint8_t *player_count, uint8_t *cell_x, uint8_t *cell_y)
{
    if (!_aim_active)
        return 0;
    *player_count = _aim_players;
    *cell_x = _aim_x;
    *cell_y = _aim_y;
    return 1;
}

void gfx_pointer_blank(uint8_t blank)
{
    if (!gfx_window || blank == _pointer_blanked)
        return;
    _pointer_blanked = blank;
    if (blank)
        SetPointer(gfx_window, _chip_blank_ptr, 1, 16, 0, 0);
    else
        ClearPointer(gfx_window);
}
