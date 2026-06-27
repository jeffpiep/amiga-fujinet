/*
 * http_get.c - Amiga FujiNet HTTP GET example
 *
 * Performs an HTTP GET request via FujiNet-NIO over RS-232.
 *
 * Build:
 *   cd apps/http_get && make
 *
 * Run on Amiga (copy http_get to SYS:):
 *   http_get http://example.com/
 *
 * Requires FujiNet running on Linux box connected via RS-232.
 * See: contracts/rs232-hardware.md
 */

#include <stdio.h>
#include <string.h>
#include "fujinet-nio.h"

#ifndef FN_DEFAULT_URL
#define FN_DEFAULT_URL "http://fujinet.online/"
#endif

#define BUFFER_SIZE 512
static uint8_t buf[BUFFER_SIZE];

int main(int argc, char *argv[])
{
    const char *url;
    fn_handle_t handle;
    uint8_t result;
    uint16_t http_status;
    uint32_t content_length;
    uint8_t info_flags;
    uint16_t bytes_read;
    uint8_t flags;
    uint32_t total_read;

    url = (argc > 1) ? argv[1] : FN_DEFAULT_URL;

    printf("FujiNet-NIO HTTP GET\n");
    printf("URL: %s\n\n", url);

    result = fn_init();
    if (result != FN_OK) {
        printf("Init failed: %s\n", fn_error_string(result));
        return 1;
    }

    if (!fn_is_ready()) {
        printf("FujiNet not ready.\n");
        return 1;
    }

    uint8_t open_flags = 0;
    if (strncmp(url, "https://", 8) == 0 || strncmp(url, "tls://", 6) == 0)
        open_flags |= FN_OPEN_TLS;

    result = fn_open(&handle, FN_METHOD_GET, url, open_flags);
    if (result != FN_OK) {
        printf("Open failed: %s\n", fn_error_string(result));
        return 1;
    }

    result = fn_info(handle, &http_status, &content_length, &info_flags);
    if (result == FN_OK) {
        if (info_flags & FN_INFO_HAS_STATUS)
            printf("HTTP %u\n", http_status);
        if (info_flags & FN_INFO_HAS_LENGTH)
            printf("Length: %lu\n", (unsigned long)content_length);
    }

    printf("\n");
    total_read = 0;

    for (;;) {
        result = fn_read(handle, total_read, buf, BUFFER_SIZE - 1,
                         &bytes_read, &flags);

        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY)
            continue;

        if (result != FN_OK) {
            printf("\nRead error: %s\n", fn_error_string(result));
            break;
        }

        if (bytes_read == 0)
            break;

        buf[bytes_read < BUFFER_SIZE ? bytes_read : BUFFER_SIZE - 1] = '\0';
        printf("%s", (char *)buf);
        total_read += bytes_read;

        if (flags & FN_READ_EOF)
            break;
    }

    printf("\n\n%lu bytes.\n", (unsigned long)total_read);

    fn_close(handle);
    return 0;
}
