/*
 * fujinet-fuji.h implementation for Amiga.
 *
 * AppKeys (fuji_set_appkey_details / fuji_read_appkey / fuji_write_appkey)
 * are NOT implemented here: fujinet-nio-lib provides them natively in its
 * src/legacy/ archive members (include/fn_legacy_appkey.h), storing keys as
 * persist:///FujiNet/<creator><app><key>.key via the FileDevice — the same
 * layout classic firmware used. See contracts/fujinet-appkey-protocol.md.
 * The linker resolves those three symbols from fujinet-nio-amiga.a.
 *
 * Everything left in this file is inert stubs for the rest of the
 * fujinet-fuji.h surface. No AmigaOS headers.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "amiga_compat.h"   /* must come before fujinet-fuji.h */
#include "fujinet-fuji.h"
#include "fujinet-nio.h"

FNStatus _fuji_status;

/* Legacy fujinet-lib exposed a global error flag; the nio-lib legacy
 * appkey API reports errors through return values only. */
bool fuji_error(void)
{
    return false;
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
