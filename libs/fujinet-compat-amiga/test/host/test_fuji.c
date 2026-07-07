/*
 * test_fuji.c — T1 host unit test for the appkey → app-store mapping in
 * fn_fuji.c (contracts/fujinet-appkey-protocol.md).
 *
 * Runs natively (`cc`), no AmigaOS, no FujiNet backend. Includes the module
 * source directly (single translation unit) and provides *scripted* stubs for
 * the two fn_appstore_* calls it uses, capturing arguments so the tests can
 * verify the namespace/key naming convention and the classic-API semantics.
 *
 * See docs/testing.md.
 */
#define FN_TEST_MAIN
#include "fn_test.h"

#include <stdint.h>
#include <string.h>

/* The module under test. Pulls in fujinet-fuji.h + fujinet-nio.h via -I. */
#include "../../src/fn_fuji.c"

/* ------------------------------------------------------------------ */
/* Scripted stubs for fn_appstore_read / fn_appstore_write.            */
/* Each test sets the stub_* script, calls the API, then inspects the  */
/* captured cap_* arguments.                                           */
/* ------------------------------------------------------------------ */

#define STUB_ERR 0x99  /* any non-FN_OK transport result */

static char     cap_ns[64];
static char     cap_key[64];
static uint32_t cap_offset;
static uint16_t cap_max_len;
static uint8_t  cap_wdata[128];
static uint16_t cap_wlen;
static int      read_calls, write_calls;

static uint8_t  stub_read_result;
static uint8_t  stub_read_flags;
static uint16_t stub_read_bytes;
static uint8_t  stub_read_data[128];
static uint8_t  stub_write_result;
static int      stub_write_short;  /* report one byte fewer than sent */

static void stub_reset(void)
{
    memset(cap_ns, 0, sizeof(cap_ns));
    memset(cap_key, 0, sizeof(cap_key));
    cap_offset = 0; cap_max_len = 0; cap_wlen = 0;
    read_calls = write_calls = 0;
    stub_read_result = FN_OK;
    stub_read_flags  = 0;
    stub_read_bytes  = 0;
    stub_write_result = FN_OK;
    stub_write_short  = 0;
}

uint8_t fn_appstore_read(const char *namespace_name, const char *key,
                         uint32_t offset, uint8_t *buf, uint16_t max_len,
                         fn_appstore_read_t *out)
{
    ++read_calls;
    snprintf(cap_ns, sizeof(cap_ns), "%s", namespace_name);
    snprintf(cap_key, sizeof(cap_key), "%s", key);
    cap_offset  = offset;
    cap_max_len = max_len;

    if (stub_read_result != FN_OK)
        return stub_read_result;

    out->flags      = stub_read_flags;
    out->offset     = offset;
    out->bytes_read = stub_read_bytes;
    if (stub_read_bytes)
        memcpy(buf, stub_read_data,
               stub_read_bytes < max_len ? stub_read_bytes : max_len);
    return FN_OK;
}

uint8_t fn_appstore_write(const char *namespace_name, const char *key,
                          uint32_t offset, const uint8_t *data, uint16_t len,
                          fn_appstore_write_t *out)
{
    ++write_calls;
    snprintf(cap_ns, sizeof(cap_ns), "%s", namespace_name);
    snprintf(cap_key, sizeof(cap_key), "%s", key);
    cap_offset = offset;
    cap_wlen   = len;
    memcpy(cap_wdata, data, len < sizeof(cap_wdata) ? len : sizeof(cap_wdata));

    if (stub_write_result != FN_OK)
        return stub_write_result;

    out->offset        = offset;
    out->bytes_written = stub_write_short ? (uint16_t)(len - 1) : len;
    return FN_OK;
}

/* ------------------------------------------------------------------ */
/* Tests                                                              */
/* ------------------------------------------------------------------ */

static void test_naming_convention(void)
{
    uint8_t buf[MAX_APPKEY_LEN];
    uint16_t count;

    /* Battleship prefs: creator 0xE41C, app 0x05, key 0x00 */
    stub_reset();
    stub_read_flags = FN_APPSTORE_READ_EXISTS;
    fuji_set_appkey_details(0xE41C, 0x05, DEFAULT);
    CHECK(fuji_read_appkey(0x00, &count, buf));
    CHECK_EQ(strcmp(cap_ns, "appkey-e41c-05"), 0);
    CHECK_EQ(strcmp(cap_key, "00"), 0);

    /* Lobby: creator 0x0001, app 0x01, key 0x05 — leading zeros kept */
    stub_reset();
    stub_read_flags = FN_APPSTORE_READ_EXISTS;
    fuji_set_appkey_details(0x0001, 0x01, DEFAULT);
    CHECK(fuji_read_appkey(0x05, &count, buf));
    CHECK_EQ(strcmp(cap_ns, "appkey-0001-01"), 0);
    CHECK_EQ(strcmp(cap_key, "05"), 0);
}

static void test_read_existing(void)
{
    uint8_t buf[MAX_APPKEY_LEN];
    uint16_t count = 0xFFFF;

    stub_reset();
    stub_read_flags = FN_APPSTORE_READ_EXISTS;
    stub_read_bytes = 5;
    memcpy(stub_read_data, "HELLO", 5);

    fuji_set_appkey_details(0xE41C, 0x05, DEFAULT);
    CHECK(fuji_read_appkey(0x00, &count, buf));
    CHECK_EQ(count, 5);
    CHECK_EQ(memcmp(buf, "HELLO", 5), 0);
    CHECK(!fuji_error());

    /* single chunk from offset 0, 64-byte ceiling */
    CHECK_EQ(cap_offset, 0);
    CHECK_EQ(cap_max_len, MAX_APPKEY_LEN);
    CHECK_EQ(read_calls, 1);
}

static void test_read_missing_key_defaults(void)
{
    uint8_t buf[MAX_APPKEY_LEN];
    uint16_t count;

    /* Known key: lobby player name defaults to "amiga" */
    stub_reset();  /* flags = 0 → key does not exist */
    fuji_set_appkey_details(0x0001, 0x01, DEFAULT);
    CHECK(fuji_read_appkey(0x00, &count, buf));
    CHECK_EQ(count, 5);
    CHECK_EQ(memcmp(buf, "amiga", 5), 0);
    CHECK(!fuji_error());

    /* Known key: battleship prefs default has seenHelp (byte 1) set */
    stub_reset();
    fuji_set_appkey_details(0xE41C, 0x05, DEFAULT);
    CHECK(fuji_read_appkey(0x00, &count, buf));
    CHECK_EQ(count, 24);
    CHECK_EQ(buf[1], 1);

    /* Unknown key: success with count 0 (absent key is not an error) */
    stub_reset();
    fuji_set_appkey_details(0x1234, 0x02, DEFAULT);
    count = 0xFFFF;
    CHECK(fuji_read_appkey(0x07, &count, buf));
    CHECK_EQ(count, 0);
    CHECK(!fuji_error());
}

static void test_read_transport_error(void)
{
    uint8_t buf[MAX_APPKEY_LEN];
    uint16_t count = 0xFFFF;

    stub_reset();
    stub_read_result = STUB_ERR;
    fuji_set_appkey_details(0xE41C, 0x05, DEFAULT);
    CHECK(!fuji_read_appkey(0x00, &count, buf));
    CHECK_EQ(count, 0);
    CHECK(fuji_error());
}

static void test_write(void)
{
    uint8_t data[MAX_APPKEY_LEN + 1];
    memset(data, 0xAB, sizeof(data));

    /* Happy path: offset 0, exact byte count, captured payload matches */
    stub_reset();
    fuji_set_appkey_details(0x0001, 0x01, DEFAULT);
    CHECK(fuji_write_appkey(0x00, 8, data));
    CHECK(!fuji_error());
    CHECK_EQ(cap_offset, 0);
    CHECK_EQ(cap_wlen, 8);
    CHECK_EQ(memcmp(cap_wdata, data, 8), 0);
    CHECK_EQ(strcmp(cap_ns, "appkey-0001-01"), 0);
    CHECK_EQ(strcmp(cap_key, "00"), 0);

    /* Oversize write rejected client-side, transport never called */
    stub_reset();
    CHECK(!fuji_write_appkey(0x00, MAX_APPKEY_LEN + 1, data));
    CHECK(fuji_error());
    CHECK_EQ(write_calls, 0);

    /* Short write reported by device → failure */
    stub_reset();
    stub_write_short = 1;
    CHECK(!fuji_write_appkey(0x00, 8, data));
    CHECK(fuji_error());

    /* Transport error → failure */
    stub_reset();
    stub_write_result = STUB_ERR;
    CHECK(!fuji_write_appkey(0x00, 8, data));
    CHECK(fuji_error());
}

int main(void)
{
    test_naming_convention();
    test_read_existing();
    test_read_missing_key_defaults();
    test_read_transport_error();
    test_write();
    return fn_test_report("test_fuji");
}
