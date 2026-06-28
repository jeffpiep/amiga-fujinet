#include "misc.h"

void initGraphics(void) {}
void resetGraphics(void) {}
void resetScreen(void) {}
void waitvsync(void) {}

uint8_t cycleNextColor(void) { return 0; }

void drawText(uint8_t x, uint8_t y, const char *s)    { (void)x; (void)y; (void)s; }
void drawTextAlt(uint8_t x, uint8_t y, const char *s) { (void)x; (void)y; (void)s; }
void drawIcon(uint8_t x, uint8_t y, uint8_t icon)     { (void)x; (void)y; (void)icon; }

void drawBlank(uint8_t x, uint8_t y)              { (void)x; (void)y; }
void drawSpace(uint8_t x, uint8_t y, uint8_t w)   { (void)x; (void)y; (void)w; }
void drawLine(uint8_t x, uint8_t y, uint8_t w)    { (void)x; (void)y; (void)w; }
void drawBox(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { (void)x; (void)y; (void)w; (void)h; }
void drawBoard(uint8_t playerCount)               { (void)playerCount; }
void drawClock(void)                               {}
void drawConnectionIcon(bool show)                 { (void)show; }
void drawEndgameMessage(const char *msg)           { (void)msg; }

void drawPlayerName(uint8_t player, const char *name, bool active)
    { (void)player; (void)name; (void)active; }

void drawShip(uint8_t quadrant, uint8_t size, uint8_t pos, bool hide)
    { (void)quadrant; (void)size; (void)pos; (void)hide; }

void drawLegendShip(uint8_t player, uint8_t index, uint8_t size, uint8_t status)
    { (void)player; (void)index; (void)size; (void)status; }

void drawGamefield(uint8_t quadrant, uint8_t *field)
    { (void)quadrant; (void)field; }

void drawGamefieldUpdate(uint8_t quadrant, uint8_t *gamefield, uint8_t attackPos, uint8_t anim)
    { (void)quadrant; (void)gamefield; (void)attackPos; (void)anim; }

void drawGamefieldCursor(uint8_t quadrant, uint8_t x, uint8_t y, uint8_t *gamefield, uint8_t blink)
    { (void)quadrant; (void)x; (void)y; (void)gamefield; (void)blink; }

bool saveScreenBuffer(void)  { return false; }
void restoreScreenBuffer(void) {}
