# FujiBus Protocol Contract

**Source:** `fujinet-nio/docs/protocol_reference.md` (authoritative)
**Summary for:** Amiga transport implementers in `fujinet-nio-lib`

---

## Protocol Stack

```
[ SLIP Frame ]   ← physical boundary on the serial wire
    ↓
[ FujiBus Packet ]  ← device/command/params/payload
    ↓
[ Device semantics ]
```

---

## SLIP Framing

| Symbol       | Value | Escaped As |
|--------------|-------|------------|
| `SLIP_END`   | 0xC0  | DB DC      |
| `SLIP_ESC`   | 0xDB  | DB DD      |
| others       | —     | literal    |

Frame format: `C0 <escaped-payload> C0`

- Every packet is wrapped with a leading and trailing `0xC0`
- If payload bytes are `0xC0` or `0xDB`, escape them per table above

---

## FujiBus Packet Layout (after SLIP decode)

```
+------------------+
| Header (6 bytes) |
+------------------+
| Descriptors      |  (1+ bytes)
+------------------+
| Parameters       |  (variable, little-endian)
+------------------+
| Payload          |  (optional, opaque bytes)
+------------------+
```

### Header (6 bytes)

| Offset | Field    | Type    | Description                        |
|--------|----------|---------|------------------------------------|
| 0      | device   | uint8   | Destination device ID              |
| 1      | command  | uint8   | Command code                       |
| 2–3    | length   | uint16  | Total packet length incl. header   |
| 4      | checksum | uint8   | Checksum (computed with this=0)    |
| 5      | descr    | uint8   | First descriptor byte              |

### Checksum Algorithm

```c
uint8_t checksum(const uint8_t *buf, uint16_t len) {
    uint16_t sum = 0;
    for (uint16_t i = 0; i < len; i++) {
        sum += buf[i];
        sum = (sum >> 8) + (sum & 0xFF);
    }
    return (uint8_t)sum;
}
```
Compute over entire packet with `header.checksum = 0`.

### Descriptor Encoding

Descriptor byte: `bit7=1` means another descriptor follows.

```
numFieldsTable[8] = {0,1,2,3,4,1,2,1}
fieldSizeTable[8] = {0,1,1,1,1,2,2,4}  (bytes per field)
```

Parameters are written in order, little-endian.

---

## Known Device IDs

| Device   | ID   |
|----------|------|
| Network  | 0xFD |
| Clock    | 0x45 |

(Full list in `fujinet-nio-lib/include/fn_protocol.h`)

---

## Transport Responsibilities (Amiga side)

1. Call `fn_transport_init()` once to open `serial.device`
2. For each request, call `fn_transport_exchange()`:
   - SLIP-encode the FujiBus request packet
   - Send to FujiNet via serial
   - Read bytes until closing `0xC0` received (or timeout)
   - SLIP-decode the response
   - Validate checksum
3. Return decoded response to caller

## Error Handling

- **Invalid checksum** → drop packet, return `FN_ERR_IO`
- **Timeout** → return `FN_ERR_TIMEOUT`
- **Partial frame** → buffer and keep reading until `0xC0`
- **Unknown device** → FujiNet returns `StatusCode::DeviceNotFound` in response
