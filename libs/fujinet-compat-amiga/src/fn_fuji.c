/*
 * fujinet-fuji.h implementation for Amiga via dos.library file I/O.
 *
 * AppKeys are stored as flat binary files:
 *   ENVARC:fujinet/<creator_hex>/<app_hex>/<key_hex>
 *
 * dos.library Open/Read/Write/Close are KS 1.3 safe.
 * CreateDir is used to create parent directories as needed.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <proto/dos.h>
#include <dos/dos.h>

#include "amiga_compat.h"   /* must come before fujinet-fuji.h */
#include "fujinet-fuji.h"

/* ------------------------------------------------------------------ */
/* Static context set by fuji_set_appkey_details()                   */
/* ------------------------------------------------------------------ */

static uint16_t s_creator_id = 0;
static uint8_t  s_app_id     = 0;
static uint8_t  s_last_error = 0; /* true = last op had error */

FNStatus _fuji_status;

/* ------------------------------------------------------------------ */
/* Path construction                                                  */
/* ------------------------------------------------------------------ */

/* Build ENVARC:fujinet/<creator_hex>/<app_hex>/<key_hex> into buf.   */
static void build_appkey_path(char *buf, size_t buflen,
                               uint8_t key_id)
{
    snprintf(buf, buflen,
             "ENVARC:fujinet/%04X/%02X/%02X",
             (unsigned)s_creator_id,
             (unsigned)s_app_id,
             (unsigned)key_id);
}

/* Build parent dir path (ENVARC:fujinet/<creator>/<app>) into buf.   */
static void build_appkey_dir(char *buf, size_t buflen)
{
    snprintf(buf, buflen,
             "ENVARC:fujinet/%04X/%02X",
             (unsigned)s_creator_id,
             (unsigned)s_app_id);
}

/* Ensure ENVARC:fujinet/<creator>/<app> exists, creating as needed.  */
static bool ensure_appkey_dir(void)
{
    char base[64];
    char dir[80];

    snprintf(base, sizeof(base), "ENVARC:fujinet/%04X",
             (unsigned)s_creator_id);
    build_appkey_dir(dir, sizeof(dir));

    /* CreateDir returns NULL and sets IoErr if dir already exists —  */
    /* that is fine; only fail if it's a real error.                  */
    BPTR lock = Lock(base, ACCESS_READ);
    if (!lock)
        CreateDir(base);
    else
        UnLock(lock);

    lock = Lock(dir, ACCESS_READ);
    if (!lock) {
        BPTR newdir = CreateDir(dir);
        if (!newdir)
            return false;
        UnLock(newdir);
    } else {
        UnLock(lock);
    }

    return true;
}

/* ------------------------------------------------------------------ */
/* Public API — Battleship-required functions                         */
/* ------------------------------------------------------------------ */

void fuji_set_appkey_details(uint16_t creator_id, uint8_t app_id,
                              enum AppKeySize keysize)
{
    (void)keysize; /* only DEFAULT (64 bytes) supported */
    s_creator_id = creator_id;
    s_app_id     = app_id;
    s_last_error = 0;
}

bool fuji_write_appkey(uint8_t key_id, uint16_t count, uint8_t *data)
{
    char path[64];
    BPTR fh;
    LONG written;

    if (!ensure_appkey_dir()) {
        s_last_error = 1;
        return false;
    }

    build_appkey_path(path, sizeof(path), key_id);

    fh = Open(path, MODE_NEWFILE);
    if (!fh) {
        s_last_error = 1;
        return false;
    }

    written = Write(fh, data, (LONG)count);
    Close(fh);

    if (written != (LONG)count) {
        s_last_error = 1;
        return false;
    }

    s_last_error = 0;
    return true;
}

bool fuji_read_appkey(uint8_t key_id, uint16_t *count, uint8_t *data)
{
    char path[64];
    BPTR fh;
    LONG got;

    build_appkey_path(path, sizeof(path), key_id);

    fh = Open(path, MODE_OLDFILE);
    if (!fh) {
        /* File doesn't exist or ENVARC: not mounted.
         * Provide built-in defaults for known keys so the game can start
         * in a bare KS 1.3 environment without a full Workbench ENVARC. */
        if (s_creator_id == 1 && s_app_id == 1 && key_id == 0) {
            /* Lobby player name: default to "amiga" */
            static const uint8_t def[] = "amiga";
            *count = sizeof(def) - 1;
            memcpy(data, def, *count);
            s_last_error = 0;
            return true;
        }
        if (s_creator_id == 0xE41C && s_app_id == 5 && key_id == 0) {
            /* Prefs: byte 1 = seenHelp = 1 so help screen is skipped */
            static const uint8_t def[24] = { 0x00, 0x01, 0x00, 0x00 };
            *count = sizeof(def);
            memcpy(data, def, *count);
            s_last_error = 0;
            return true;
        }
        *count = 0;
        s_last_error = 1;
        return false;
    }

    got = Read(fh, data, MAX_APPKEY_LEN);
    Close(fh);

    if (got < 0) {
        *count = 0;
        s_last_error = 1;
        return false;
    }

    *count       = (uint16_t)got;
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
