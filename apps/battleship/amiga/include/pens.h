/*
 * pens.h - battleship's pen numbers for the 16-color screen
 *
 * The gamekit takes a palette as data and knows nothing about what the
 * colors mean; the names live here, next to the art that uses them. Colors
 * come from tile_palette[] in tiles.h — tune the RGB4 there, not here.
 *
 * See: docs/plan-track1b-battleship.md (Phase 3c),
 *      docs/plan-track1c-fujitzee.md (Phase 0 extraction)
 */
#ifndef PENS_H
#define PENS_H

#define PEN_BG       0   /* black background            */
#define PEN_TEXT     1   /* white text                  */
#define PEN_SEA      2   /* sea blue                    */
#define PEN_HIT      3   /* hit red                     */
#define PEN_SHIP     4   /* ship green                  */
#define PEN_TEXT_ALT 5   /* amber alt/emphasis text     */
#define PEN_DIM      6   /* grey inactive/dim           */
#define PEN_SEA_DK   7   /* dark blue sea detail        */
#define PEN_EXPL     8   /* orange explosion            */
#define PEN_CONN     9   /* cyan connection icon        */
/* Art-pass shade pens — free slots the multicolor tiles draw with. */
#define PEN_SHIP_HI  10  /* ship hull highlight / deck  */
#define PEN_SHIP_SHD 11  /* ship hull shadow            */
#define PEN_SEA_LT   12  /* water light / wave crest    */
#define PEN_FOAM     13  /* white foam / bright detail  */
#define PEN_WOOD     14  /* deck / superstructure wood  */
#define PEN_SPARE    15  /* reserved                    */

#endif /* PENS_H */
