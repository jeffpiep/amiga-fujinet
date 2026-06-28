/*
 * compat_test — smoke test for libfn_compat_amiga
 *
 * Tests:
 *   1. HTTP GET via network_open / network_read / network_close
 *   2. AppKey round-trip via fuji_set_appkey_details /
 *      fuji_write_appkey / fuji_read_appkey
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "fujinet-network.h"
#include "amiga_compat.h"   /* NewDisk for Amiga before fujinet-fuji.h */
#include "fujinet-fuji.h"
#undef FN_ERR_UNKNOWN
#include "fujinet-nio.h"

#define TEST_URL "N:HTTP://httpbin.org/get"
#define APPKEY_CREATOR 0x0001u
#define APPKEY_APP     0x01u
#define APPKEY_KEY     0x00u

static int test_http_get(void)
{
    uint8_t  buf[512];
    int16_t  got;
    uint8_t  err;
    int      total = 0;

    printf("[http] network_open... ");
    err = network_open(TEST_URL, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE);
    if (err != FN_ERR_OK) {
        printf("FAIL (err=%d)\n", (int)err);
        return 1;
    }
    printf("OK\n");

    printf("[http] network_read loop...\n");
    for (;;) {
        got = network_read(TEST_URL, buf, sizeof(buf) - 1);
        if (got <= 0)
            break;
        buf[got] = '\0';
        printf("%s", (char *)buf);
        total += got;
    }
    printf("\n[http] total bytes: %d\n", total);

    printf("[http] network_close... ");
    err = network_close(TEST_URL);
    printf("%s\n", (err == FN_ERR_OK) ? "OK" : "FAIL");

    return (total > 0) ? 0 : 1;
}

static int test_appkey_roundtrip(void)
{
    const char *payload    = "testuser";
    uint8_t     wdata[MAX_APPKEY_LEN];
    uint8_t     rdata[MAX_APPKEY_LEN + 2];
    uint16_t    count      = 0;
    bool        ok;

    memset(wdata, 0, sizeof(wdata));
    memset(rdata, 0, sizeof(rdata));
    memcpy(wdata, payload, strlen(payload));

    printf("[appkey] fuji_set_appkey_details... ");
    fuji_set_appkey_details(APPKEY_CREATOR, APPKEY_APP, DEFAULT);
    printf("OK\n");

    printf("[appkey] fuji_write_appkey '%s'... ", payload);
    ok = fuji_write_appkey(APPKEY_KEY, (uint16_t)strlen(payload), wdata);
    printf("%s\n", ok ? "OK" : "FAIL");
    if (!ok) return 1;

    printf("[appkey] fuji_read_appkey... ");
    ok = fuji_read_appkey(APPKEY_KEY, &count, rdata);
    printf("%s (count=%u)\n", ok ? "OK" : "FAIL", (unsigned)count);
    if (!ok) return 1;

    rdata[count] = '\0';
    printf("[appkey] value='%s'\n", (char *)rdata);

    if (count != (uint16_t)strlen(payload) ||
        memcmp(rdata, payload, count) != 0) {
        printf("[appkey] MISMATCH: expected '%s'\n", payload);
        return 1;
    }

    printf("[appkey] round-trip OK\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("=== fujinet-compat-amiga smoke test ===\n\n");

    printf("-- fn_init --\n");
    if (fn_init() != FN_OK) {
        printf("fn_init failed — is fujinet-nio running?\n");
        return 1;
    }
    printf("fn_init OK\n\n");

    printf("-- HTTP GET test --\n");
    failures += test_http_get();
    printf("\n");

    printf("-- AppKey round-trip test --\n");
    failures += test_appkey_roundtrip();
    printf("\n");

    if (failures == 0)
        printf("ALL TESTS PASSED\n");
    else
        printf("%d TEST(S) FAILED\n", failures);

    return failures;
}
