/*
 * cellmap.h - battleship board layout math (pure C99, no AmigaOS headers)
 *
 * Maps game-logic coordinates (quadrant, cell, field codes) onto the 40x25
 * cell grid of the 320x200 graphical renderer, and picks tile ids for field
 * contents and ship segments. Mirrors the Atari renderer's layout
 * (upstream/src/atari/graphics.c) squeezed from 26 to 25 rows: top boards at
 * row 2, bottom boards at row 13, sharing border row 12.
 *
 * Pure logic: T1 host-testable (test/host/test_cellmap.c).
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c)
 */
#ifndef CELLMAP_H
#define CELLMAP_H

#include <stdint.h>

/* Cell grid: 8x8-pixel cells on a 320x200 screen. */
#define CM_CELL_W 8
#define CM_CELL_H 8
#define CM_PX(cx) ((int16_t)(cx) * CM_CELL_W)
#define CM_PY(cy) ((int16_t)(cy) * CM_CELL_H)

/* Field cell codes — must match FIELD_ATTACK / FIELD_MISS in upstream
 * misc.h (checked with #if in graphics.c). */
#define CM_FIELD_ATTACK 1
#define CM_FIELD_MISS   2

/*
 * Tile ids. The art lives in tiles.h (tile_table[] indexed by these ids);
 * cellmap only names them so layout decisions stay host-testable. Extend at
 * the end; TILE_COUNT sizes the chip-RAM bank.
 */
enum {
    TILE_BLANK = 0,
    TILE_SEA,
    TILE_MISS,
    TILE_HIT,
    TILE_HIT2,          /* hit blink alternate */
    TILE_LEGEND_HIT,    /* destroyed ship in legend drawer */

    TILE_SHIP_BOW_H,    /* horizontal ship: bow / middle / stern */
    TILE_SHIP_MID_H,
    TILE_SHIP_STERN_H,
    TILE_SHIP_BOW_V,    /* vertical ship: top / middle / bottom */
    TILE_SHIP_MID_V,
    TILE_SHIP_STERN_V,

    TILE_ANIM_0,        /* attack animation, anim codes 10..15 */
    TILE_ANIM_1,
    TILE_ANIM_2,
    TILE_ANIM_3,
    TILE_ANIM_4,
    TILE_ANIM_5,

    TILE_BORDER_H,      /* board / drawer frame pieces */
    TILE_BORDER_V,
    TILE_BOX_TL,
    TILE_BOX_TR,
    TILE_BOX_BL,
    TILE_BOX_BR,

    TILE_LINE,          /* drawLine() bar */
    TILE_ENDGAME_BAR,   /* drawEndgameMessage() separator row */
    TILE_CLOCK,
    TILE_CONN_ON,
    TILE_CONN_OFF,

    TILE_COUNT
};

/* Number of ship slots in a legend drawer (upstream fleet size). */
#define CM_LEGEND_SLOTS 5

/* Horizontal centering offset: boards shift right by 7 cells when only two
 * boards are shown (Atari fieldX). */
uint8_t cm_field_x(uint8_t player_count);

/* Top-left cell of quadrant q's 10x10 grid. Quadrants: 0 bottom-left,
 * 1 top-left, 2 top-right, 3 bottom-right (Atari convention). */
void cm_quadrant_origin(uint8_t player_count, uint8_t quadrant,
                        uint8_t *cx, uint8_t *cy);

/* Legend drawer: 3x8-cell panel beside each board. Players 0..1 get a left
 * drawer, 2..3 a right drawer (with 2 players, player 1 is right). */
uint8_t cm_drawer_is_right(uint8_t player_count, uint8_t player);
void cm_drawer_origin(uint8_t player_count, uint8_t player,
                      uint8_t *cx, uint8_t *cy);

/* Cell offset of legend ship <index> (0..CM_LEGEND_SLOTS-1) inside its
 * drawer. Ships are drawn vertically. */
void cm_legend_offset(uint8_t index, uint8_t *dx, uint8_t *dy);

/* Tile for segment i (0-based) of a size-cell ship. */
uint8_t cm_ship_tile(uint8_t i, uint8_t size, uint8_t vertical);

/* Tile overlaying a gamefield cell during a full drawGamefield() pass. */
uint8_t cm_field_tile(uint8_t cell);

/* Tile for drawGamefieldUpdate(): anim 10..15 selects an explosion frame,
 * anim 1..9 blinks a fresh hit (TILE_HIT2), anim 0 shows the final state. */
uint8_t cm_update_tile(uint8_t cell, uint8_t anim);

#endif /* CELLMAP_H */
