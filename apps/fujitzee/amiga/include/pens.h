/*
 * pens.h - fujitzee's pen numbers for the 16-color screen
 *
 * The gamekit takes a palette as data and knows nothing about what the
 * colors mean; the names live here, next to the art that uses them. Colors
 * come from tile_palette[] in tiles.h — tune the RGB4 there, not here.
 *
 * Same split as apps/battleship/amiga/include/pens.h, deliberately: the two
 * ports share the engine, not the palette.
 *
 * See: docs/plan-track1c-fujitzee.md (Phase 2)
 */
#ifndef PENS_H
#define PENS_H

#define PEN_BG        0   /* table background                             */
#define PEN_TEXT      1   /* white body text                              */
#define PEN_ALT       2   /* amber alt text (drawTextAlt)                 */
#define PEN_LINE      3   /* board lattice rules and boxes                */

#define PEN_DIE       4   /* plain die face                               */
#define PEN_DIE_KEEP  5   /* held die face                                */
#define PEN_DIE_HI    6   /* fujitzee flash / pressed Roll button face    */
#define PEN_PIP       7   /* pips and die outline (ink on the face)       */

#define PEN_HI_SELF   8   /* active-column bracket, local player's turn   */
#define PEN_HI_OTHER  9   /* active-column bracket, someone else's turn   */
#define PEN_HI_FLASH 10   /* active-column bracket, end-of-turn flash     */

#define PEN_CURSOR   11   /* dice selection cursor brackets               */
#define PEN_CONN     12   /* connection-trouble icon                      */
#define PEN_DIM      13   /* inactive/secondary text                      */
/* Free slots the Phase 3c art pass can draw multicolor tiles with. */
#define PEN_SHADE    14
#define PEN_SPARE    15

#endif /* PENS_H */
