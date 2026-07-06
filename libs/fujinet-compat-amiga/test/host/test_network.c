/*
 * test_network.c — T1 host unit test for the pure-logic helpers in fn_network.c.
 *
 * Runs natively (`cc`), no AmigaOS, no FujiNet backend. It includes the module
 * source directly (single translation unit) so the file-local `static` helpers
 * (strip_prefix, map_mode, map_err) are reachable without changing production
 * code. The transport-dependent network_* functions come along for the ride, so
 * we provide inert linker stubs for the fn_* symbols they reference. We do NOT
 * test transport behavior here — that is the job of the T2 emulator smoke test.
 *
 * See docs/testing.md.
 */
#define FN_TEST_MAIN
#include "fn_test.h"

/* The module under test. Pulls in fujinet-network.h + fujinet-nio.h via -I. */
#include "../../src/fn_network.c"

/* ------------------------------------------------------------------ */
/* Inert stubs: satisfy the linker for fn_* transport symbols that     */
/* fn_network.c's network_* functions reference. Never called by the   */
/* pure-logic tests below.                                             */
/* ------------------------------------------------------------------ */
uint8_t fn_init(void) { return FN_OK; }
uint8_t fn_open(fn_handle_t *h, uint8_t m, const char *u, uint8_t f)
{ (void)m; (void)u; (void)f; if (h) *h = 0; return FN_OK; }
uint8_t fn_close(fn_handle_t h) { (void)h; return FN_OK; }
uint8_t fn_read(fn_handle_t h, uint32_t o, uint8_t *b, uint16_t n,
                uint16_t *got, uint8_t *fl)
{ (void)h; (void)o; (void)b; (void)n; if (got) *got = 0; if (fl) *fl = 0; return FN_OK; }
uint8_t fn_write(fn_handle_t h, uint32_t o, const uint8_t *b, uint16_t n, uint16_t *w)
{ (void)h; (void)o; (void)b; (void)n; if (w) *w = n; return FN_OK; }
uint8_t fn_info(fn_handle_t h, uint16_t *s, uint32_t *cl, uint8_t *fl)
{ (void)h; if (s) *s = 0; if (cl) *cl = 0; if (fl) *fl = 0; return FN_OK; }

/* ------------------------------------------------------------------ */
/* Tests                                                              */
/* ------------------------------------------------------------------ */

static void test_strip_prefix(void)
{
    /* "N:" and "n:" strip two chars. */
    CHECK_EQ(strcmp(strip_prefix("N:HTTP://host/x"), "HTTP://host/x"), 0);
    CHECK_EQ(strcmp(strip_prefix("n:HTTP://host/x"), "HTTP://host/x"), 0);

    /* "N<1-9>:" strips three chars. */
    CHECK_EQ(strcmp(strip_prefix("N3:HTTP://host/x"), "HTTP://host/x"), 0);

    /* "N0:" — 0 is not in 1-9, so this is passthrough (documents real behavior). */
    CHECK_EQ(strcmp(strip_prefix("N0:HTTP://x"), "N0:HTTP://x"), 0);

    /* No N-prefix at all — passthrough. */
    CHECK_EQ(strcmp(strip_prefix("HTTP://host/x"), "HTTP://host/x"), 0);

    /* Bare "N" (no colon, no digit) — passthrough, must not read past end. */
    CHECK_EQ(strcmp(strip_prefix("N"), "N"), 0);
}

static void test_map_mode(void)
{
    CHECK_EQ(map_mode(OPEN_MODE_HTTP_GET),    FN_METHOD_GET);
    CHECK_EQ(map_mode(OPEN_MODE_HTTP_POST),   FN_METHOD_POST);
    CHECK_EQ(map_mode(OPEN_MODE_HTTP_PUT),    FN_METHOD_PUT);
    CHECK_EQ(map_mode(OPEN_MODE_HTTP_DELETE), FN_METHOD_DELETE);
    /* Unknown mode falls back to GET. */
    CHECK_EQ(map_mode(0xFF), FN_METHOD_GET);
}

static void test_map_err(void)
{
    CHECK_EQ(map_err(FN_OK),            FN_ERR_OK);
    CHECK_EQ(map_err(FN_ERR_NOT_FOUND), FN_ERR_NO_DEVICE);
    CHECK_EQ(map_err(FN_ERR_INVALID),   FN_ERR_BAD_CMD);
    CHECK_EQ(map_err(FN_ERR_BUSY),      FN_ERR_WARNING);
    CHECK_EQ(map_err(FN_ERR_NOT_READY), FN_ERR_WARNING);
    CHECK_EQ(map_err(FN_ERR_IO),        FN_ERR_IO_ERROR);
    CHECK_EQ(map_err(FN_ERR_TIMEOUT),   FN_ERR_IO_ERROR);
    /* Unmapped code falls through to UNKNOWN. */
    CHECK_EQ(map_err(0x7E), FN_ERR_UNKNOWN);
}

int main(void)
{
    test_strip_prefix();
    test_map_mode();
    test_map_err();
    return fn_test_report("test_network");
}
