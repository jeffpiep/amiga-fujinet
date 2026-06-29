/*
 * fujinet-network.h implementation for Amiga via fujinet-nio-lib.
 *
 * devicespec format: "N[1-4]:PROTO://host/path"
 * The N: prefix and optional unit digit are stripped before calling fn_open().
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Include network header first; it defines FN_ERR_UNKNOWN (0xff).
 * fujinet-nio.h defines FN_ERR_UNKNOWN with the same value — suppress
 * the redefinition warning by undefining before the second include. */
#include "fujinet-network.h"
#undef FN_ERR_UNKNOWN
#include "fujinet-nio.h"

/* ------------------------------------------------------------------ */
/* Globals declared in fujinet-network.h                              */
/* ------------------------------------------------------------------ */

uint16_t fn_bytes_read   = 0;
uint8_t  fn_device_error = 0;
uint16_t fn_network_bw   = 0;
uint8_t  fn_network_conn = 0;
uint8_t  fn_network_error = 0;

/* ------------------------------------------------------------------ */
/* Internal slot table                                                */
/* ------------------------------------------------------------------ */

typedef struct {
    fn_handle_t handle;
    char        devicespec[256];
    uint32_t    rx_cursor;
    uint8_t     in_use;
} NetSlot;

static NetSlot slots[FN_MAX_SESSIONS];

static NetSlot *slot_find(const char *devicespec)
{
    int i;
    for (i = 0; i < FN_MAX_SESSIONS; i++) {
        if (slots[i].in_use &&
            strncmp(slots[i].devicespec, devicespec, 255) == 0)
            return &slots[i];
    }
    return NULL;
}

static NetSlot *slot_alloc(void)
{
    int i;
    for (i = 0; i < FN_MAX_SESSIONS; i++) {
        if (!slots[i].in_use)
            return &slots[i];
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Strip leading "N:" or "N<digit>:" prefix, return pointer into s. */
static const char *strip_prefix(const char *devicespec)
{
    if (devicespec[0] == 'N' || devicespec[0] == 'n') {
        if (devicespec[1] == ':')
            return devicespec + 2;
        if (devicespec[1] >= '1' && devicespec[1] <= '9' &&
            devicespec[2] == ':')
            return devicespec + 3;
    }
    return devicespec;
}

static uint8_t map_mode(uint8_t mode)
{
    switch (mode) {
    case OPEN_MODE_HTTP_GET:    return FN_METHOD_GET;
    case OPEN_MODE_HTTP_POST:   return FN_METHOD_POST;
    case OPEN_MODE_HTTP_PUT:    return FN_METHOD_PUT;
    case OPEN_MODE_HTTP_DELETE: return FN_METHOD_DELETE;
    default:                    return FN_METHOD_GET;
    }
}

/* Map fujinet-nio error codes to fujinet-network error codes. */
static uint8_t map_err(uint8_t nio_err)
{
    switch (nio_err) {
    case FN_OK:             return FN_ERR_OK;
    case FN_ERR_NOT_FOUND:  return FN_ERR_NO_DEVICE;
    case FN_ERR_INVALID:    return FN_ERR_BAD_CMD;
    case FN_ERR_BUSY:
    case FN_ERR_NOT_READY:  return FN_ERR_WARNING;
    case FN_ERR_IO:         return FN_ERR_IO_ERROR;
    case FN_ERR_TIMEOUT:    return FN_ERR_IO_ERROR;
    default:                return FN_ERR_UNKNOWN;
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

uint8_t network_init(void)
{
    uint8_t err = fn_init();
    fn_device_error = err;
    return map_err(err);
}

uint8_t network_open(const char *devicespec, uint8_t mode, uint8_t trans)
{
    NetSlot    *slot;
    fn_handle_t handle;
    const char *url;
    uint8_t     method;
    uint8_t     flags = 0;
    uint8_t     err;

    (void)trans; /* translation modes not applicable on Amiga */

    /* Lazy init: battleship never calls network_init() explicitly */
    fn_init();

    slot = slot_find(devicespec);
    if (slot) {
        /* already open — close and reopen */
        fn_close(slot->handle);
        memset(slot, 0, sizeof(*slot));
    }

    slot = slot_alloc();
    if (!slot) {
        fn_device_error = FN_ERR_NO_HANDLES;
        return FN_ERR_IO_ERROR;
    }

    url    = strip_prefix(devicespec);
    method = map_mode(mode);

    err = fn_open(&handle, method, url, flags);
    fn_device_error = err;
    if (err != FN_OK)
        return map_err(err);

    slot->handle    = handle;
    slot->rx_cursor = 0;
    slot->in_use    = 1;
    strncpy(slot->devicespec, devicespec, 255);
    slot->devicespec[255] = '\0';

    return FN_ERR_OK;
}

uint8_t network_close(const char *devicespec)
{
    NetSlot *slot = slot_find(devicespec);
    if (!slot)
        return FN_ERR_OK; /* already closed */

    uint8_t err = fn_close(slot->handle);
    fn_device_error = err;
    memset(slot, 0, sizeof(*slot));
    return map_err(err);
}

int16_t network_read_nb(const char *devicespec, uint8_t *buf, uint16_t len)
{
    NetSlot  *slot;
    uint16_t  got  = 0;
    uint8_t   rflags = 0;
    uint8_t   err;

    fn_bytes_read = 0;

    slot = slot_find(devicespec);
    if (!slot) {
        fn_network_error = FN_ERR_NO_DEVICE;
        return -(int16_t)FN_ERR_NO_DEVICE;
    }

    err = fn_read(slot->handle, slot->rx_cursor, buf, len, &got, &rflags);
    fn_device_error = err;

    if (err == FN_ERR_NOT_READY || err == FN_ERR_BUSY) {
        fn_network_error = FN_ERR_OK;
        fn_bytes_read = 0;
        return 0;
    }

    if (err != FN_OK) {
        fn_network_error = map_err(err);
        fn_bytes_read = 0;
        return -(int16_t)map_err(err);
    }

    slot->rx_cursor += got;
    fn_bytes_read    = got;
    fn_network_conn  = (rflags & FN_READ_EOF) ? 0 : 1;
    fn_network_error = FN_ERR_OK;

    return (int16_t)got;
}

int16_t network_read(const char *devicespec, uint8_t *buf, uint16_t len)
{
    NetSlot  *slot;
    uint16_t  total = 0;
    uint16_t  got;
    uint8_t   rflags;
    uint8_t   err;

    fn_bytes_read = 0;

    slot = slot_find(devicespec);
    if (!slot) {
        fn_network_error = FN_ERR_NO_DEVICE;
        return -(int16_t)FN_ERR_NO_DEVICE;
    }

    while (total < len) {
        got    = 0;
        rflags = 0;

        err = fn_read(slot->handle, slot->rx_cursor,
                      buf + total, len - total, &got, &rflags);
        fn_device_error = err;

        if (err == FN_ERR_NOT_READY || err == FN_ERR_BUSY) {
            /* spin — caller wants blocking semantics */
            continue;
        }

        if (err != FN_OK) {
            fn_network_error = map_err(err);
            fn_bytes_read    = total;
            return (total > 0) ? (int16_t)total : -(int16_t)map_err(err);
        }

        slot->rx_cursor += got;
        total           += got;

        if (rflags & FN_READ_EOF)
            break;

        if (got == 0)
            break;
    }

    fn_bytes_read    = total;
    fn_network_conn  = (total < len) ? 0 : 1;
    fn_network_error = FN_ERR_OK;

    return (int16_t)total;
}

uint8_t network_write(const char *devicespec, const uint8_t *buf, uint16_t len)
{
    NetSlot  *slot;
    uint16_t  written = 0;
    uint8_t   err;

    slot = slot_find(devicespec);
    if (!slot) {
        fn_device_error = FN_ERR_NOT_FOUND;
        return FN_ERR_NO_DEVICE;
    }

    err = fn_write(slot->handle, 0, buf, len, &written);
    fn_device_error = err;
    return map_err(err);
}

uint8_t network_status(const char *devicespec,
                       uint16_t *bw, uint8_t *c, uint8_t *err)
{
    NetSlot  *slot;
    uint16_t  http_status   = 0;
    uint32_t  content_length = 0;
    uint8_t   info_flags    = 0;
    uint8_t   rc;

    slot = slot_find(devicespec);
    if (!slot) {
        *bw  = 0;
        *c   = 0;
        *err = FN_ERR_NO_DEVICE;
        return FN_ERR_NO_DEVICE;
    }

    rc = fn_info(slot->handle, &http_status, &content_length, &info_flags);
    fn_device_error = rc;

    if (rc == FN_ERR_NOT_READY) {
        *bw  = 0;
        *c   = 1; /* still connected, data not yet available */
        *err = FN_ERR_OK;
        return FN_ERR_OK;
    }

    if (rc != FN_OK) {
        *bw  = 0;
        *c   = 0;
        *err = map_err(rc);
        return map_err(rc);
    }

    /* bytes waiting: use content_length if known, else 0 (unknown) */
    *bw  = (content_length > 0xFFFF) ? 0xFFFF : (uint16_t)content_length;
    *c   = (info_flags & FN_INFO_CONNECTED) ? 1 : 0;
    *err = FN_ERR_OK;

    fn_network_bw    = *bw;
    fn_network_conn  = *c;
    fn_network_error = FN_ERR_OK;

    return FN_ERR_OK;
}

uint8_t network_unit(const char *devicespec)
{
    if (devicespec[0] == 'N' || devicespec[0] == 'n') {
        if (devicespec[1] >= '1' && devicespec[1] <= '9')
            return (uint8_t)(devicespec[1] - '0');
    }
    return 1; /* default unit */
}

/* ------------------------------------------------------------------ */
/* HTTP convenience wrappers                                          */
/* ------------------------------------------------------------------ */

uint8_t network_http_post(const char *devicespec, const char *data)
{
    return network_write(devicespec, (const uint8_t *)data, (uint16_t)strlen(data));
}

uint8_t network_http_post_bin(const char *devicespec,
                               const uint8_t *data, uint16_t len)
{
    return network_write(devicespec, data, len);
}

uint8_t network_http_put(const char *devicespec, const char *data)
{
    return network_write(devicespec, (const uint8_t *)data, (uint16_t)strlen(data));
}

uint8_t network_http_delete(const char *devicespec, uint8_t trans)
{
    (void)trans;
    return network_open(devicespec, OPEN_MODE_HTTP_DELETE, 0);
}

/* ------------------------------------------------------------------ */
/* Stubs — not needed for Battleship                                  */
/* ------------------------------------------------------------------ */

uint8_t fn_error(uint8_t code) { return map_err(code); }

uint8_t network_ioctl(uint8_t cmd, uint8_t aux1, uint8_t aux2,
                      const char *devicespec, ...)
{
    (void)cmd; (void)aux1; (void)aux2; (void)devicespec;
    return FN_ERR_BAD_CMD;
}

uint8_t network_json_parse(const char *devicespec)
{
    (void)devicespec;
    return FN_ERR_BAD_CMD;
}

int16_t network_json_query(const char *devicespec,
                            const char *query, char *s)
{
    (void)devicespec; (void)query;
    if (s) s[0] = '\0';
    return -(int16_t)FN_ERR_BAD_CMD;
}

uint8_t network_http_set_channel_mode(const char *devicespec, uint8_t mode)
{
    (void)devicespec; (void)mode;
    return FN_ERR_BAD_CMD;
}

uint8_t network_http_start_add_headers(const char *devicespec)
{
    (void)devicespec;
    return FN_ERR_BAD_CMD;
}

uint8_t network_http_end_add_headers(const char *devicespec)
{
    (void)devicespec;
    return FN_ERR_BAD_CMD;
}

uint8_t network_http_add_header(const char *devicespec, const char *header)
{
    (void)devicespec; (void)header;
    return FN_ERR_BAD_CMD;
}

uint8_t network_fs_delete(const char *devicespec)  { (void)devicespec; return FN_ERR_BAD_CMD; }
uint8_t network_fs_rename(const char *devicespec)  { (void)devicespec; return FN_ERR_BAD_CMD; }
uint8_t network_fs_lock(const char *devicespec)    { (void)devicespec; return FN_ERR_BAD_CMD; }
uint8_t network_fs_unlock(const char *devicespec)  { (void)devicespec; return FN_ERR_BAD_CMD; }
uint8_t network_fs_mkdir(const char *devicespec)   { (void)devicespec; return FN_ERR_BAD_CMD; }
uint8_t network_fs_rmdir(const char *devicespec)   { (void)devicespec; return FN_ERR_BAD_CMD; }
uint8_t network_fs_cd(const char *devicespec)      { (void)devicespec; return FN_ERR_BAD_CMD; }
