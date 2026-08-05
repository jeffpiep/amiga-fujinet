/*
 * gfxcore.c - screen/window/tile plumbing for graphical Amiga game ports
 *
 * OS-friendly custom screen: Intuition owns the copper list and display
 * DMA, multitasking stays alive so serial I/O to fujinet-nio keeps running
 * underneath. All rendering goes through the backdrop window's RastPort,
 * so graphics.library clips it via the layer — no manual bounds paranoia.
 *
 * Blitter/sprite source data MUST live in chip RAM: the data hunk can land
 * in slow RAM (FS-UAE's default A500), so everything the caller hands us in
 * struct gfx_config is staged into one AllocMem(MEMF_CHIP) bank at init.
 * The .datachip linker section does NOT work on this toolchain — see
 * apps/pacmantests/c99-pac-man/README.md.
 *
 * Requires: intuition.library, graphics.library (V33 / KS 1.3)
 * Compiler: m68k-amigaos-gcc (amiga-gcc)
 *
 * Extracted from apps/battleship/amiga in Track 1C Phase 0 —
 * see docs/plan-track1c-fujitzee.md.
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

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct Window *gfx_window;

static struct Screen *_screen;

/* Retained config — the caller guarantees it outlives gfx_close(). */
static const struct gfx_config *_cfg;

/* Derived sizes, computed once at open so the draw path stays multiply-free
 * where it can be (68000: every 32-bit multiply is a subroutine call). */
static ULONG _tile_bytes;      /* GFX_CELL_H * depth * sizeof(UWORD)       */
static ULONG _ras_bytes;       /* one bitplane of the save buffer          */
static ULONG _chip_bank_bytes;

/* Sprite overlay state. Slots map onto hardware sprites cfg->sprite_first
 * upward (sprite 0 is the Intuition pointer). */
static struct SimpleSprite _spr[GFX_SPRITE_SLOTS_MAX];
static BYTE _spr_got[GFX_SPRITE_SLOTS_MAX] = { -1, -1, -1, -1 };
/* Frames since each slot last moved; parked once it reaches park_age. */
static uint8_t _spr_age[GFX_SPRITE_SLOTS_MAX];

/* Chip-RAM staging bank: all tiles, per-slot sprite images (each slot needs
 * its own copies — MoveSprite writes posctl words into the attached data),
 * and a zeroed blank mouse-pointer sprite. */
#define BLANK_PTR_WORDS  6
#define BLANK_PTR_BYTES  (BLANK_PTR_WORDS * sizeof(UWORD))
static UWORD *_chip_bank;
static UWORD *_chip_tiles;
static UWORD *_chip_spr[GFX_SPRITE_SLOTS_MAX][2];   /* [slot][alt] */
static UWORD *_chip_blank_ptr;

static uint8_t _pointer_blanked;

/* Spare bitmap for gfx_save_screen()/gfx_restore_screen(). */
static struct BitMap _save_bm;
static PLANEPTR _save_planes;          /* one chunk, depth * _ras_bytes */

/* Explicit topaz 8: the *system default* font can be the 60-column topaz
 * (10 px wide), which overflows the 8x8 cell grid and misaligns every
 * string against the tile layout. TOPAZ_EIGHTY is always in ROM. */
static struct TextAttr _topaz8 = {
    (STRPTR)"topaz.font", TOPAZ_EIGHTY, FS_NORMAL, FPF_ROMFONT
};

static struct NewScreen _new_screen = {
    0, 0, 0, 0, 0,     /* dims/depth filled in from cfg at open */
    0, 1,
    0,                 /* ViewModes: lores */
    CUSTOMSCREEN,
    &_topaz8,          /* 8x8 font to match the cell grid */
    NULL,              /* no title — ShowTitle(FALSE) anyway */
    NULL, NULL
};

static struct NewWindow _new_window = {
    0, 0, 0, 0,        /* dims filled in from cfg at open */
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
    0, 0, GFX_CELL_W, GFX_CELL_H, 0, NULL, 0x00, 0x00, NULL
};

/* Y to park a sprite at: below the visible raster, clear of the display. */
static WORD spr_park_y(void)
{
    return (WORD)(_cfg->screen_h + 16);
}

void gfx_close(void)
{
    uint8_t i;

    if (_cfg) {
        for (i = 0; i < _cfg->sprite_slots; i++) {
            if (_spr_got[i] >= 0) {
                FreeSprite(_cfg->sprite_first + i);
                _spr_got[i] = -1;
            }
        }
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
        FreeMem(_save_planes, (ULONG)_cfg->depth * _ras_bytes);
        _save_planes = NULL;
    }
    if (_chip_bank) {
        FreeMem(_chip_bank, _chip_bank_bytes);
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
    _cfg = NULL;
}

uint8_t gfx_open(const struct gfx_config *cfg)
{
    uint8_t i;
    UWORD *dst;
    ULONG spr_bytes;

    if (!cfg || !cfg->palette || !cfg->tiles || !cfg->depth)
        return 0;
    if (cfg->sprite_slots > GFX_SPRITE_SLOTS_MAX)
        return 0;

    _cfg = cfg;
    _tile_bytes = (ULONG)GFX_CELL_H * cfg->depth * sizeof(UWORD);
    _ras_bytes  = (ULONG)(cfg->screen_w / 8) * cfg->screen_h;
    spr_bytes   = (ULONG)cfg->sprite_words * sizeof(UWORD);
    _chip_bank_bytes = (ULONG)cfg->tile_count * _tile_bytes +
                       (ULONG)cfg->sprite_slots * 2 * spr_bytes +
                       BLANK_PTR_BYTES;

    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    if (!GfxBase)
        goto fail;
    IntuitionBase = (struct IntuitionBase *)
        OpenLibrary((CONST_STRPTR)"intuition.library", 0);
    if (!IntuitionBase)
        goto fail;

    /* Stage every tile and both sprite images per slot into chip RAM. */
    _chip_bank = (UWORD *)AllocMem(_chip_bank_bytes, MEMF_CHIP);
    if (!_chip_bank)
        goto fail;
    dst = _chip_bank;
    _chip_tiles = dst;
    for (i = 0; i < cfg->tile_count; i++) {
        CopyMem((APTR)cfg->tiles[i], dst, _tile_bytes);
        dst += _tile_bytes / sizeof(UWORD);
    }
    for (i = 0; i < cfg->sprite_slots; i++) {
        _chip_spr[i][0] = dst;
        CopyMem((APTR)cfg->sprite_img_a, dst, spr_bytes);
        dst += cfg->sprite_words;
        _chip_spr[i][1] = dst;
        CopyMem((APTR)cfg->sprite_img_b, dst, spr_bytes);
        dst += cfg->sprite_words;
    }
    _chip_blank_ptr = dst;
    memset(_chip_blank_ptr, 0, BLANK_PTR_BYTES);   /* invisible pointer */

    /* Save/restore buffer: the blitter reads it back on restore, so it
     * must be chip RAM like everything else the blitter touches. */
    _save_planes = (PLANEPTR)AllocMem((ULONG)cfg->depth * _ras_bytes, MEMF_CHIP);
    if (!_save_planes)
        goto fail;
    InitBitMap(&_save_bm, cfg->depth, cfg->screen_w, cfg->screen_h);
    for (i = 0; i < cfg->depth; i++)
        _save_bm.Planes[i] = _save_planes + (ULONG)i * _ras_bytes;

    _new_screen.Width  = (WORD)cfg->screen_w;
    _new_screen.Height = (WORD)cfg->screen_h;
    _new_screen.Depth  = (WORD)cfg->depth;
    _screen = OpenScreen(&_new_screen);
    if (!_screen)
        goto fail;
    ShowTitle(_screen, FALSE);
    LoadRGB4(&_screen->ViewPort, (UWORD *)cfg->palette, 1 << cfg->depth);

    _tile_img.Depth     = (WORD)cfg->depth;
    _tile_img.PlanePick = (UBYTE)((1u << cfg->depth) - 1u);

    _new_window.Width  = (WORD)cfg->screen_w;
    _new_window.Height = (WORD)cfg->screen_h;
    _new_window.Screen = _screen;
    gfx_window = OpenWindow(&_new_window);
    if (!gfx_window)
        goto fail;

    /* Overlay sprites. Sprite pairs share color registers (2/3 -> 21-23,
     * 4/5 -> 25-27): register base for a pair is 16 + pair * 4 + 1. Load
     * the sprite colors into every bank a slot lands in. A failed GetSprite
     * just costs that slot its overlay — the game stays playable, so don't
     * treat it as fatal. */
    for (i = 0; i < cfg->sprite_slots; i++) {
        uint8_t sprnum = cfg->sprite_first + i;
        uint8_t base = 16 + (sprnum / 2) * 4 + 1;
        uint8_t c;

        _spr[i].x = 0;
        _spr[i].y = 0;
        _spr[i].height = cfg->sprite_height;
        _spr[i].num = 0;
        _spr_got[i] = GetSprite(&_spr[i], sprnum);
        if (_spr_got[i] >= 0)
            ChangeSprite(&_screen->ViewPort, &_spr[i], (APTR)_chip_spr[i][0]);

        if (!cfg->sprite_rgb)
            continue;
        for (c = 0; c < 3; c++)
            SetRGB4(&_screen->ViewPort, base + c,
                    (cfg->sprite_rgb[c] >> 8) & 0xF,
                    (cfg->sprite_rgb[c] >> 4) & 0xF,
                    cfg->sprite_rgb[c] & 0xF);
    }
    gfx_sprite_hide();

    atexit(gfx_close);
    return 1;

fail:
    gfx_close();
    return 0;
}

void gfx_draw_tile(uint8_t cx, uint8_t cy, uint8_t tile)
{
    if (!_cfg || !gfx_window)
        return;
    if (cx >= _cfg->grid_w || cy >= _cfg->grid_h || tile >= _cfg->tile_count)
        return;
    _tile_img.ImageData = _chip_tiles + (ULONG)tile * (_tile_bytes / sizeof(UWORD));
    DrawImage(gfx_window->RPort, &_tile_img,
              (WORD)cx * GFX_CELL_W, (WORD)cy * GFX_CELL_H);
}

void gfx_text(uint8_t cx, uint8_t cy, const char *s, uint8_t pen, uint8_t bpen)
{
    struct RastPort *rp;
    WORD len;

    if (!_cfg || !gfx_window || !s)
        return;
    if (cy >= _cfg->grid_h)
        return;
    len = (WORD)strlen(s);
    if (len <= 0)
        return;
    /* Clip to the row so a long string can't wrap into layer clipping
     * weirdness (the layer clips pixels; we clip characters). */
    if (cx >= _cfg->grid_w)
        return;
    if (cx + len > _cfg->grid_w)
        len = _cfg->grid_w - cx;

    rp = gfx_window->RPort;
    SetAPen(rp, pen);
    SetBPen(rp, bpen);
    SetDrMd(rp, JAM2);
    Move(rp, (WORD)cx * GFX_CELL_W, (WORD)cy * GFX_CELL_H + rp->TxBaseline);
    Text(rp, (CONST_STRPTR)s, (ULONG)len);
}

void gfx_fill(uint8_t cx, uint8_t cy, uint8_t w, uint8_t h, uint8_t pen)
{
    struct RastPort *rp;

    if (!_cfg || !gfx_window || !w || !h)
        return;
    if (cx >= _cfg->grid_w || cy >= _cfg->grid_h)
        return;
    if (cx + w > _cfg->grid_w)
        w = _cfg->grid_w - cx;
    if (cy + h > _cfg->grid_h)
        h = _cfg->grid_h - cy;

    rp = gfx_window->RPort;
    SetAPen(rp, pen);
    SetDrMd(rp, JAM1);
    RectFill(rp, (WORD)cx * GFX_CELL_W, (WORD)cy * GFX_CELL_H,
             (WORD)(cx + w) * GFX_CELL_W - 1, (WORD)(cy + h) * GFX_CELL_H - 1);
}

void gfx_sprite_move(uint8_t slot, uint8_t cx, uint8_t cy, uint8_t alt)
{
    if (!_cfg || slot >= _cfg->sprite_slots || _spr_got[slot] < 0)
        return;
    _spr_age[slot] = 0;
    ChangeSprite(&_screen->ViewPort, &_spr[slot],
                 (APTR)_chip_spr[slot][alt ? 1 : 0]);
    MoveSprite(&_screen->ViewPort, &_spr[slot],
               (WORD)cx * GFX_CELL_W, (WORD)cy * GFX_CELL_H);
}

void gfx_sprite_hide(void)
{
    uint8_t i;

    if (!_cfg)
        return;
    for (i = 0; i < _cfg->sprite_slots; i++) {
        if (_spr_got[i] >= 0) {
            _spr_age[i] = _cfg->sprite_park_age;
            MoveSprite(&_screen->ViewPort, &_spr[i], 0, spr_park_y());
        }
    }
}

void gfx_sprite_sweep(void)
{
    uint8_t i;

    if (!_cfg || !_cfg->sprite_park_age)
        return;
    for (i = 0; i < _cfg->sprite_slots; i++) {
        if (_spr_got[i] < 0 || _spr_age[i] >= _cfg->sprite_park_age)
            continue;
        if (++_spr_age[i] == _cfg->sprite_park_age)
            MoveSprite(&_screen->ViewPort, &_spr[i], 0, spr_park_y());
    }
}

uint8_t gfx_save_screen(void)
{
    if (!_screen || !_save_planes)
        return 0;
    BltBitMap(&_screen->BitMap, 0, 0, &_save_bm, 0, 0,
              _cfg->screen_w, _cfg->screen_h, 0xC0, 0xFF, NULL);
    return 1;
}

void gfx_restore_screen(void)
{
    if (!_screen || !_save_planes)
        return;
    BltBitMap(&_save_bm, 0, 0, &_screen->BitMap, 0, 0,
              _cfg->screen_w, _cfg->screen_h, 0xC0, 0xFF, NULL);
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
