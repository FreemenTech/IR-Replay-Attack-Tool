# IR Replay Attack v2 (with EEPROM Persistence)

This version adds **non-volatile signal storage** to the IR Replay Attack tool. The captured signal survives power loss and reboots. Unplug the Arduino, plug it back in, press the button, the TV still turns off.

For full documentation on the IR replay attack (circuit, protocols, attack flow, bugs), see [v1 README](../).

## What Changed from v1

| | v1 | v2 |
|---|---|---|
| Storage | RAM only | RAM + EEPROM |
| After reboot | Signal lost, must recapture | Signal loaded automatically |
| New dependency | None | `#include <EEPROM.h>` |
| New functions | None | `saveToEEPROM()`, `loadFromEEPROM()` |

## What is EEPROM

**Electrically Erasable Programmable Read-Only Memory.** A small non-volatile memory inside the ATmega328P microcontroller (the chip on Arduino UNO). Unlike RAM, EEPROM keeps its data when power is cut.

| Spec | Value |
|------|-------|
| Total size | 1024 bytes |
| Write endurance | 100,000 writes per address |
| Read endurance | Unlimited |
| Data retention | ~100 years |
| Write speed | ~3.3ms per byte |
| Read speed | Instant |

## Memory Map

Each captured signal uses 6 bytes:

```
Address 0-1 : lastAddress  (uint16_t, 2 bytes)
Address 2-3 : lastCommand  (uint16_t, 2 bytes)
Address 4   : protocol     (uint8_t, 1 byte)
Address 5   : flag         (0xAA = valid data exists)
```

**Total: 6 bytes out of 1024 available.**

### Why a flag byte?

Fresh EEPROM contains `0xFF` in every address. Without a flag, `loadFromEEPROM()` would read garbage data on first boot and treat it as a valid signal. The flag byte at address 5 is set to `0xAA` only after a successful save. On boot, if the flag is not `0xAA`, we know no valid signal exists.

`0xAA` is `10101010` in binary. It was chosen because it's unlikely to appear as random EEPROM noise (which tends to be `0xFF` or `0x00`).

## How It Works

### Save flow (on signal capture)

```
New IR signal received
    |
    v
Store in RAM (same as v1)
    |
    v
EEPROM.put(0, address)     -> writes 2 bytes at addresses 0-1
EEPROM.put(2, command)     -> writes 2 bytes at addresses 2-3
EEPROM.write(4, protocol)  -> writes 1 byte at address 4
EEPROM.write(5, 0xAA)      -> sets validity flag
    |
    v
Serial: "[EEPROM] Signal saved!"
```

### Load flow (on boot)

```
Arduino powers on
    |
    v
setup() calls loadFromEEPROM()
    |
    v
Read flag at address 5
    |
    ├── flag == 0xAA -> valid signal exists
    |       |
    |       v
    |   EEPROM.get(0, address)
    |   EEPROM.get(2, command)
    |   protocol = EEPROM.read(4)
    |   hasSignal = true
    |       |
    |       v
    |   Ready to replay immediately (no capture needed)
    |
    └── flag != 0xAA -> no saved signal
            |
            v
        Wait for first capture (same as v1)
```

## EEPROM API

Two approaches, both used in this code:

| Function | Use case | Example |
|----------|----------|---------|
| `EEPROM.write(addr, byte)` | Single byte (uint8_t) | `EEPROM.write(4, protocol)` |
| `EEPROM.read(addr)` | Read single byte | `EEPROM.read(5)` |
| `EEPROM.put(addr, variable)` | Multi-byte types (uint16_t, struct) | `EEPROM.put(0, lastAddress)` |
| `EEPROM.get(addr, variable)` | Read multi-byte types | `EEPROM.get(0, lastAddress)` |

`put()` and `get()` automatically handle the byte count based on the variable's type. A `uint16_t` writes 2 bytes, a `struct` writes its full size.

### put() vs write()

`EEPROM.put()` is smarter than `write()`. It only writes bytes that actually changed. This extends EEPROM lifespan because unchanged addresses don't consume write cycles. If you capture the same signal twice, `put()` skips the write entirely.

## Write Endurance

Each EEPROM address survives 100,000 write cycles. In this code, every new capture writes to the same 6 addresses (0-5).

| Captures per day | Lifespan |
|------------------|----------|
| 10 | ~27 years |
| 100 | ~2.7 years |
| 1000 | ~100 days |

For a security tool used in testing, this is more than enough. For a production device writing frequently, you'd implement **wear leveling** (rotating write addresses across the full 1024 bytes).

## Security Angle

### Why EEPROM matters for IoT security

EEPROM is where IoT devices store **credentials, keys, and configuration**. This includes:

- WiFi passwords on ESP8266/ESP32
- Encryption keys on access control systems
- Device pairing codes
- Calibration data on sensors

**Attack implications:**

1. **Physical extraction** : with physical access to an IoT device, EEPROM contents can be dumped via JTAG, SWD, or by reading the chip directly. No encryption by default
2. **Persistence** : malware that writes to EEPROM survives firmware updates and factory resets (if the reset doesn't explicitly clear EEPROM)
3. **Forensics** : EEPROM retains data even after the device is "wiped." Previous signals, credentials, or states can be recovered

### This tool as a persistence demo

v1 loses its payload on power cycle. An attacker would need to recapture the target signal every time. v2 demonstrates **persistent access**: capture once, replay forever, even across reboots. This mirrors real-world persistence techniques in embedded malware.

## What I Learned

1. EEPROM provides non-volatile storage without external hardware (SD card, flash chip)
2. A validity flag prevents reading garbage data from uninitialized memory
3. `put()`/`get()` handle multi-byte types automatically and skip unchanged bytes
4. EEPROM has a finite write lifespan that matters for production but not for prototyping
5. Persistent storage turns a one-session tool into a persistent attack capability

## Author

**Freemen Houngbedji** : Cybersecurity student. Building toward IoT security
