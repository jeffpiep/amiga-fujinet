# Track 1A Gap Table — fujinet-network.h / fujinet-fuji.h vs fujinet-nio-lib

Generated from upstream `FujiNetWIFI/fujinet-lib` headers.

Legend:
- **Direct map** — maps cleanly to an existing `fn_*` call
- **Needs work** — requires logic or a new nio protocol command
- **Stub / skip** — not needed for Battleship; safe to return `false`/`0` for now

---

## fujinet-network.h

| Function | Status | Notes |
|----------|--------|-------|
| `network_init()` | Direct map | → `fn_init()` |
| `network_open(devicespec, mode, trans)` | Direct map | Strip `N:` prefix, map `OPEN_MODE_*` to `FN_METHOD_*`, store handle in slot table |
| `network_close(devicespec)` | Direct map | Slot lookup → `fn_close()` |
| `network_read(devicespec, buf, len)` | Direct map | Slot lookup → `fn_read()` loop until EOF or len satisfied |
| `network_read_nb(devicespec, buf, len)` | Direct map | Slot lookup → single `fn_read()` call, no retry |
| `network_write(devicespec, buf, len)` | Direct map | Slot lookup → `fn_write()` |
| `network_status(devicespec, bw, c, err)` | Needs work | Slot lookup → `fn_info()`; map HTTP status/content-length to bw/conn/error |
| `network_unit(devicespec)` | Direct map | Parse the `N<x>:` digit from devicespec string |
| `network_http_post(devicespec, data)` | Direct map | Slot lookup → `fn_write()` (connection already open in POST mode) |
| `network_http_post_bin(devicespec, data, len)` | Direct map | Same as above, binary variant |
| `network_http_put(devicespec, data)` | Direct map | Same pattern |
| `network_http_delete(devicespec, trans)` | Direct map | `fn_open()` with `FN_METHOD_DELETE` |
| `network_ioctl(cmd, aux1, aux2, devicespec, ...)` | Stub / skip | SIO-specific control commands; no equivalent in FujiBus |
| `network_json_parse(devicespec)` | Stub / skip | Not needed for Battleship |
| `network_json_query(devicespec, query, s)` | Stub / skip | Not needed for Battleship |
| `network_http_set_channel_mode(devicespec, mode)` | Stub / skip | Not needed for Battleship |
| `network_http_start_add_headers(devicespec)` | Stub / skip | Not needed for Battleship (future: would need fn_nio extension) |
| `network_http_end_add_headers(devicespec)` | Stub / skip | Not needed for Battleship |
| `network_http_add_header(devicespec, header)` | Stub / skip | Not needed for Battleship |
| `network_fs_delete(devicespec)` | Stub / skip | Filesystem ops; not needed for Battleship |
| `network_fs_rename(devicespec)` | Stub / skip | |
| `network_fs_lock(devicespec)` | Stub / skip | |
| `network_fs_unlock(devicespec)` | Stub / skip | |
| `network_fs_mkdir(devicespec)` | Stub / skip | |
| `network_fs_rmdir(devicespec)` | Stub / skip | |
| `network_fs_cd(devicespec)` | Stub / skip | |

### Global vars declared in fujinet-network.h

| Variable | Implementation |
|----------|---------------|
| `fn_bytes_read` | Maintained by `network_read` / `network_read_nb` |
| `fn_device_error` | Set from `fn_*` error codes |
| `fn_network_bw` | Set by `network_status` via `fn_info()` |
| `fn_network_conn` | Set by `network_status` |
| `fn_network_error` | Set by `network_status` |

---

## fujinet-fuji.h (Battleship-required subset)

| Function | Status | Notes |
|----------|--------|-------|
| `fuji_set_appkey_details(creator_id, app_id, keysize)` | Direct map | Stores context in static vars; no device call |
| `fuji_read_appkey(key_id, count, data)` | Needs work | Read binary file from `ENVARC:fujinet/<creator_hex>/<app_hex>/<key_hex>` via `dos.library` |
| `fuji_write_appkey(key_id, count, data)` | Needs work | Write binary file to same path; create dirs if needed |
| `fuji_error()` | Direct map | Return last operation's error status from static var |

## fujinet-fuji.h (full audit — stub everything else)

| Function | Status | Notes |
|----------|--------|-------|
| `fuji_reset()` | Stub / skip | No hardware reset on Amiga path |
| `fuji_status()` | Stub / skip | |
| `fuji_get_ssid()` / `fuji_set_ssid()` | Stub / skip | WiFi N/A on Amiga |
| `fuji_scan_for_networks()` / `fuji_get_scan_result()` | Stub / skip | WiFi N/A |
| `fuji_get_wifi_status()` / `fuji_get_wifi_enabled()` | Stub / skip | WiFi N/A |
| `fuji_get_adapter_config()` / `fuji_get_adapter_config_extended()` | Stub / skip | WiFi N/A |
| `fuji_get_host_slots()` / `fuji_put_host_slots()` | Stub / skip | Host slot management N/A |
| `fuji_mount_host_slot()` / `fuji_unmount_host_slot()` | Stub / skip | |
| `fuji_get_device_slots()` / `fuji_put_device_slots()` | Stub / skip | Device slot management N/A |
| `fuji_mount_disk_image()` / `fuji_unmount_disk_image()` | Stub / skip | |
| `fuji_mount_all()` | Stub / skip | |
| `fuji_open_directory()` / `fuji_open_directory2()` | Stub / skip | |
| `fuji_read_directory()` / `fuji_read_directory_block()` | Stub / skip | |
| `fuji_close_directory()` | Stub / skip | |
| `fuji_get_directory_position()` / `fuji_set_directory_position()` | Stub / skip | |
| `fuji_create_new()` | Stub / skip | |
| `fuji_copy_file()` | Stub / skip | |
| `fuji_get_device_filename()` / `fuji_set_device_filename()` | Stub / skip | |
| `fuji_enable_device()` / `fuji_disable_device()` / `fuji_get_device_enabled_status()` | Stub / skip | |
| `fuji_get_host_prefix()` / `fuji_set_host_prefix()` | Stub / skip | |
| `fuji_set_boot_config()` / `fuji_set_boot_mode()` / `fuji_config_boot()` | Stub / skip | |
| `fuji_enable_udpstream()` | Stub / skip | |
| `fuji_generate_guid()` | Stub / skip | |
| `fuji_hash_*` (all variants) | Stub / skip | Not needed for Battleship |
| `fuji_base64_*` (all variants) | Stub / skip | Not needed for Battleship |

---

## Summary

No surprises. The required Battleship subset (6 functions) has a clean path:
- 3 network functions → direct maps to existing `fn_*` calls via the slot table
- 1 fuji context setter → static vars, no device call
- 2 appkey I/O functions → ENVARC: file I/O via `dos.library` (KS 1.3 safe)

The full `fujinet-network.h` has 26 functions; 12 are direct maps or near-direct maps,
2 need work (`network_status`), and 12 are stub/skip. No gaps that block Battleship.
