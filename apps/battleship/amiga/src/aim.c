/* aim.c - mouse-aim handshake state (see aim.h).
 *
 * Split out of gfxcore.c when that moved to libs/amiga-gamekit: it is pure
 * battleship policy, not screen plumbing. gfx_aim_clear() also unblanks the
 * pointer, which is the one gamekit call this module makes. */
#include "aim.h"
#include "gfxcore.h"

static uint8_t _aim_mode;      /* GFX_AIM_* */
static uint8_t _aim_players;
static uint8_t _aim_x;
static uint8_t _aim_y;
static uint8_t _aim_ship_size;
static uint8_t _aim_ship_vert;

void gfx_aim_set(uint8_t player_count, uint8_t cell_x, uint8_t cell_y)
{
    _aim_mode = GFX_AIM_ATTACK;
    _aim_players = player_count;
    _aim_x = cell_x;
    _aim_y = cell_y;
}

void gfx_aim_set_place(uint8_t player_count, uint8_t cell_x, uint8_t cell_y,
                       uint8_t ship_size, uint8_t ship_vertical)
{
    _aim_mode = GFX_AIM_PLACE;
    _aim_players = player_count;
    _aim_x = cell_x;
    _aim_y = cell_y;
    _aim_ship_size = ship_size;
    _aim_ship_vert = ship_vertical;
}

void gfx_aim_clear(void)
{
    _aim_mode = GFX_AIM_NONE;
    gfx_pointer_blank(0);
}

uint8_t gfx_aim_get(uint8_t *player_count, uint8_t *cell_x, uint8_t *cell_y)
{
    if (_aim_mode == GFX_AIM_NONE)
        return GFX_AIM_NONE;
    *player_count = _aim_players;
    *cell_x = _aim_x;
    *cell_y = _aim_y;
    return _aim_mode;
}

void gfx_aim_get_ship(uint8_t *ship_size, uint8_t *ship_vertical)
{
    *ship_size = _aim_ship_size;
    *ship_vertical = _aim_ship_vert;
}
