# RS-232 Hardware Contract: Amiga ↔ Linux FujiNet

## Purpose
Defines the physical and electrical interface between an Amiga computer and the Linux box running fujinet-nio over RS-232 serial.

## Amiga RS-232 Port

The Amiga uses a 25-pin DB-25 male connector on the rear panel (all models: A500, A1200, A2000, A3000, A4000). The port is driven by the 8520 CIA chip via the Amiga's built-in UART.

### DB-25 Pinout (Amiga side, DTE)

| Pin | Signal | Direction | Description              |
|-----|--------|-----------|--------------------------|
| 2   | TXD    | Out       | Transmit Data            |
| 3   | RXD    | In        | Receive Data             |
| 4   | RTS    | Out       | Request To Send          |
| 5   | CTS    | In        | Clear To Send            |
| 6   | DSR    | In        | Data Set Ready           |
| 7   | GND    | —         | Signal Ground            |
| 8   | DCD    | In        | Data Carrier Detect      |
| 20  | DTR    | Out       | Data Terminal Ready      |
| 22  | RI     | In        | Ring Indicator           |

### Linux Serial Port (DCE)

The Linux box uses a USB-to-RS232 adapter (e.g., FTDI-based `/dev/ttyUSB0`) or a built-in DB-9 port. A DB-9 to DB-25 null-modem cable connects the two.

## Cable: Null-Modem (Amiga DB-25 ↔ Linux DB-9)

Minimum connections for 3-wire (no hardware flow control):

| Amiga DB-25 | Signal | Linux DB-9 |
|-------------|--------|------------|
| 2 (TXD)     | →      | 2 (RXD)   |
| 3 (RXD)     | ←      | 3 (TXD)   |
| 7 (GND)     | —      | 5 (GND)   |

For hardware flow control (preferred):

| Amiga DB-25 | Signal | Linux DB-9 |
|-------------|--------|------------|
| 2 (TXD)     | →      | 2 (RXD)   |
| 3 (RXD)     | ←      | 3 (TXD)   |
| 4 (RTS)     | →      | 8 (CTS)   |
| 5 (CTS)     | ←      | 7 (RTS)   |
| 7 (GND)     | —      | 5 (GND)   |
| 20 (DTR)    | →      | 6+1 (DSR+DCD) | tie together |

## Serial Parameters

| Parameter   | Value         | Notes                              |
|-------------|---------------|------------------------------------|
| Baud Rate   | 19200         | Safe default; Amiga CIA can do up to 115200 but stability varies |
| Data Bits   | 8             |                                    |
| Stop Bits   | 1             |                                    |
| Parity      | None          |                                    |
| Flow Control| None (initially) | Add RTS/CTS once basics work   |

**Prototype baud rate: 19200.** Increase to 57600 or 115200 after confirming reliability.

## Amiga serial.device Notes

- Use `serial.device` unit 0 (built-in UART)
- Open via `exec.library` `OpenDevice()` with `IOExtSer` request
- Set `io_Baud`, `io_ReadLen`, `io_WriteLen`, `io_StopBits`, `io_SerFlags`
- `SERF_XDISABLED` flag disables XON/XOFF software flow control (required for binary SLIP data)
- `SERF_RAD_BOOGIE` flag enables high-speed mode (needed for ≥57600)
- For 3-wire no-flow-control: also set `SERF_7WIRE` = 0 and configure `IOExtSer.io_SerFlags` accordingly

## Linux Side Notes

- Open `/dev/ttyUSB0` (or configured device) with `O_RDWR | O_NOCTTY | O_NONBLOCK`
- Configure with termios: 8N1, no flow control, raw mode
- Set same baud as Amiga side
- fujinet-nio reads `FN_SERIAL_PORT` env var (default: `/dev/ttyUSB0`) and `FN_SERIAL_BAUD` (default: 19200)
