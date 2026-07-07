/*
 * fujinet-fuji.h implementation for Amiga over the fujinet-nio app-store.
 *
 * AppKeys map onto namespaced app-store keys served by fujinet-nio's
 * FileDevice, per contracts/fujinet-appkey-protocol.md:
 *
 *   (creator, app, key)  →  namespace "appkey-<creator%04x>-<app%02x>",
 *                           key       "<key%02x>"
 *
 * Pure logic over fn_appstore_* — no AmigaOS headers, so this file is
 * T1 host-testable (docs/testing.md).
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "amiga_compat.h"   /* must come before fujinet-fuji.h */
#include "fujinet-fuji.h"
#include "fujinet-nio.h"

/* ------------------------------------------------------------------ */
/* Static context set by fuji_set_appkey_details()                   */
/* ------------------------------------------------------------------ */

static uint16_t s_creator_id = 0;
static uint8_t  s_app_id     = 0;
static uint8_t  s_bad_size   = 0; /* non-DEFAULT keysize requested */
static uint8_t  s_last_error = 0; /* true = last op had error */

FNStatus _fuji_status;

/* ------------------------------------------------------------------ */
/* Name construction (contracts/fujinet-appkey-protocol.md)           */
/* ------------------------------------------------------------------ */

#define APPKEY_NS_LEN  16  /* "appkey-" + 4 + "-" + 2 + NUL */
#define APPKEY_KEY_LEN 4   /* 2 hex digits + NUL */

static void build_appstore_names(uint8_t key_id,
                                 char *ns, size_t nslen,
                                 char *key, size_t keylen)
{
    snprintf(ns, nslen, "appkey-%04x-%02x",
             (unsigned)s_creator_id, (unsigned)s_app_id);
    snprintf(key, keylen, "%02x", (unsigned)key_id);
}

/* Built-in defaults for known keys so games boot sanely against a
 * fresh server (missing key is not an error — classic semantics).
 * Returns true if a default was supplied. */
static bool default_for_missing_key(uint8_t key_id,
                                    uint16_t *count, uint8_t *data)
{
    if (s_creator_id == 1 && s_app_id == 1 && key_id == 0) {
        /* Lobby player name: default to "amiga" */
        const uint8_t def[] = "amiga";
        *count = sizeof(def) - 1;
        memcpy(data, def, *count);
        return true;
    }
    if (s_creator_id == 0xE41C && s_app_id == 5 && key_id == 0) {
        /* Prefs: byte 1 = seenHelp = 1 so help screen is skipped */
        const uint8_t def[24] = { 0x00, 0x01, 0x00, 0x00 };
        *count = sizeof(def);
        memcpy(data, def, *count);
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* Public API — Battleship-required functions                         */
/* ------------------------------------------------------------------ */

void fuji_set_appkey_details(uint16_t creator_id, uint8_t app_id,
                              enum AppKeySize keysize)
{
    s_creator_id = creator_id;
    s_app_id     = app_id;
    s_bad_size   = (keysize != DEFAULT); /* only DEFAULT (64 bytes) supported */
    s_last_error = 0;
}

bool fuji_write_appkey(uint8_t key_id, uint16_t count, uint8_t *data)
{
    char ns[APPKEY_NS_LEN];
    char key[APPKEY_KEY_LEN];
    fn_appstore_write_t wr;

    if (s_bad_size || count > MAX_APPKEY_LEN) {
        s_last_error = 1;
        return false;
    }

    build_appstore_names(key_id, ns, sizeof(ns), key, sizeof(key));

    /* offset 0 creates or replaces the value */
    if (fn_appstore_write(ns, key, 0, data, count, &wr) != FN_OK ||
        wr.bytes_written != count) {
        s_last_error = 1;
        return false;
    }

    s_last_error = 0;
    return true;
}

bool fuji_read_appkey(uint8_t key_id, uint16_t *count, uint8_t *data)
{
    char ns[APPKEY_NS_LEN];
    char key[APPKEY_KEY_LEN];
    fn_appstore_read_t rr;

    if (s_bad_size) {
        *count = 0;
        s_last_error = 1;
        return false;
    }

    build_appstore_names(key_id, ns, sizeof(ns), key, sizeof(key));

    /* Single chunk: 64 bytes always fits one FujiBus frame. A value
     * larger than 64 bytes (foreign writer) is truncated at 64. */
    if (fn_appstore_read(ns, key, 0, data, MAX_APPKEY_LEN, &rr) != FN_OK) {
        *count = 0;
        s_last_error = 1;
        return false;
    }

    if (!(rr.flags & FN_APPSTORE_READ_EXISTS)) {
        if (!default_for_missing_key(key_id, count, data))
            *count = 0;
        s_last_error = 0;
        return true;
    }

    *count       = rr.bytes_read;
    s_last_error = 0;
    return true;
}

bool fuji_error(void)
{
    return s_last_error != 0;
}

/* ------------------------------------------------------------------ */
/* Stubs — everything else in fujinet-fuji.h                          */
/* ------------------------------------------------------------------ */

bool fuji_close_directory(void)                                     { return false; }
bool fuji_copy_file(uint8_t s, uint8_t d, char *sp)                { (void)s;(void)d;(void)sp; return false; }
bool fuji_create_new(NewDisk *nd)                                   { (void)nd; return false; }
bool fuji_disable_device(uint8_t d)                                 { (void)d; return false; }
bool fuji_enable_device(uint8_t d)                                  { (void)d; return false; }
bool fuji_enable_udpstream(uint16_t port, char *host)               { (void)port;(void)host; return false; }
bool fuji_generate_guid(char *buf)                                  { (void)buf; return false; }
bool fuji_get_adapter_config(AdapterConfig *ac)                     { (void)ac; return false; }
bool fuji_get_adapter_config_extended(AdapterConfigExtended *ac)    { (void)ac; return false; }
bool fuji_get_device_enabled_status(uint8_t d)                      { (void)d; return true; }
bool fuji_get_device_filename(uint8_t ds, char *buf)                { (void)ds;(void)buf; return false; }
bool fuji_get_device_slots(DeviceSlot *d, size_t sz)                { (void)d;(void)sz; return false; }
bool fuji_get_directory_position(uint16_t *pos)                     { (void)pos; return false; }
bool fuji_get_host_prefix(uint8_t hs, char *p)                      { (void)hs;(void)p; return false; }
bool fuji_get_host_slots(HostSlot *h, size_t sz)                    { (void)h;(void)sz; return false; }
bool fuji_get_scan_result(uint8_t n, SSIDInfo *si)                  { (void)n;(void)si; return false; }
bool fuji_get_ssid(NetConfig *nc)                                   { (void)nc; return false; }
bool fuji_get_wifi_enabled(void)                                    { return false; }
bool fuji_get_wifi_status(uint8_t *s)                               { (void)s; return false; }
bool fuji_mount_all(void)                                           { return false; }
bool fuji_mount_disk_image(uint8_t ds, uint8_t mode)               { (void)ds;(void)mode; return false; }
bool fuji_mount_host_slot(uint8_t hs)                               { (void)hs; return false; }
bool fuji_open_directory(uint8_t hs, char *pf)                      { (void)hs;(void)pf; return false; }
bool fuji_open_directory2(uint8_t hs, char *p, char *f)             { (void)hs;(void)p;(void)f; return false; }
bool fuji_put_device_slots(DeviceSlot *d, size_t sz)                { (void)d;(void)sz; return false; }
bool fuji_put_host_slots(HostSlot *h, size_t sz)                    { (void)h;(void)sz; return false; }
bool fuji_read_directory(uint8_t ml, uint8_t a2, char *buf)        { (void)ml;(void)a2;(void)buf; return false; }
bool fuji_read_directory_block(uint8_t rp, uint8_t gs, void *buf)  { (void)rp;(void)gs;(void)buf; return false; }
bool fuji_reset(void)                                               { return false; }
bool fuji_scan_for_networks(uint8_t *count)                        { (void)count; return false; }
bool fuji_set_boot_config(uint8_t t)                               { (void)t; return false; }
bool fuji_set_boot_mode(uint8_t m)                                  { (void)m; return false; }
bool fuji_set_device_filename(uint8_t m, uint8_t hs, uint8_t ds,
                               char *buf)                           { (void)m;(void)hs;(void)ds;(void)buf; return false; }
bool fuji_set_directory_position(uint16_t pos)                      { (void)pos; return false; }
bool fuji_set_host_prefix(uint8_t hs, char *p)                      { (void)hs;(void)p; return false; }
bool fuji_set_ssid(NetConfig *nc)                                   { (void)nc; return false; }
bool fuji_status(FNStatus *s)                                       { (void)s; return false; }
bool fuji_unmount_disk_image(uint8_t ds)                            { (void)ds; return false; }
bool fuji_unmount_host_slot(uint8_t hs)                             { (void)hs; return false; }

/* Base64 stubs */
bool fuji_base64_decode_compute(void)                               { return false; }
bool fuji_base64_decode_input(char *s, uint16_t l)                 { (void)s;(void)l; return false; }
bool fuji_base64_decode_length(unsigned long *l)                    { (void)l; return false; }
bool fuji_base64_decode_output(char *s, uint16_t l)                { (void)s;(void)l; return false; }
bool fuji_base64_encode_compute(void)                               { return false; }
bool fuji_base64_encode_input(char *s, uint16_t l)                 { (void)s;(void)l; return false; }
bool fuji_base64_encode_length(unsigned long *l)                    { (void)l; return false; }
bool fuji_base64_encode_output(char *s, uint16_t l)                { (void)s;(void)l; return false; }

/* Hash stubs */
bool fuji_hash_compute(uint8_t t)                                   { (void)t; return false; }
bool fuji_hash_compute_no_clear(uint8_t t)                          { (void)t; return false; }
bool fuji_hash_input(char *s, uint16_t l)                           { (void)s;(void)l; return false; }
bool fuji_hash_output(uint8_t ot, char *s, uint16_t l)             { (void)ot;(void)s;(void)l; return false; }
bool fuji_hash_length(uint8_t m)                                    { (void)m; return false; }
bool fuji_hash_clear(void)                                          { return false; }
bool fuji_hash_add(uint8_t *d, uint16_t l)                         { (void)d;(void)l; return false; }
bool fuji_hash_calculate(hash_alg_t t, bool h, bool dd, uint8_t *o){ (void)t;(void)h;(void)dd;(void)o; return false; }
bool fuji_hash_data(hash_alg_t t, uint8_t *in, uint16_t l,
                    bool h, uint8_t *out)                           { (void)t;(void)in;(void)l;(void)h;(void)out; return false; }
uint16_t fuji_hash_size(hash_alg_t t, bool h)                      { (void)t;(void)h; return 0; }

uint8_t fn_default_timeout = 0x0F;
