# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

MeshCore uses [PlatformIO](https://docs.platformio.org). There is no single monolithic build — every firmware is a combination of a **variant** (hardware target) and an **example application**.

### Common PlatformIO commands

```bash
# List all available build environments
sh build.sh list
# or
pio project config | grep 'env:'

# Build a specific firmware (e.g. Heltec V3 companion radio over BLE)
pio run -e Heltec_v3_companion_radio_ble

# Build and upload to connected device
pio run -e Heltec_v3_companion_radio_ble -t upload

# Open serial monitor (115200 baud)
pio device monitor -e Heltec_v3_companion_radio_ble
```

### build.sh script

```bash
# Build one firmware (FIRMWARE_VERSION must be set)
export FIRMWARE_VERSION=v1.0.0
sh build.sh build-firmware Heltec_v3_repeater

# Build all firmwares of a type
sh build.sh build-companion-firmwares
sh build.sh build-repeater-firmwares
sh build.sh build-room-server-firmwares

# Build all firmwares matching a string
sh build.sh build-matching-firmwares heltec_v3

# Suppress debug flags for release builds
export DISABLE_DEBUG=1
```

Build output goes into `out/`. ESP32 targets produce both a regular `.bin` and a `-merged.bin` (for fresh flashing). NRF52 targets produce `.uf2`/`.zip`. STM32 produces `.bin`/`.hex`. RP2040 produces `.bin`/`.uf2`.

## Architecture

### Layer hierarchy

```
Dispatcher        — low-level: radio I/O, packet queue scheduling, duty-cycle enforcement
    └── Mesh      — routing layer: flood vs direct routing, ACKs, hop filtering, packet type dispatch
         └── BaseChatMesh  — application layer: contacts, messages, group channels, connections
              └── (example apps: companion_radio, simple_repeater, simple_room_server, etc.)
```

- **`Dispatcher`** (`src/Dispatcher.h/cpp`): Owns the `Radio`, `PacketManager`, and `MillisecondClock` abstractions. Handles raw receive/transmit scheduling, CAD (Channel Activity Detection), airtime budgeting, and duty-cycle limiting. Its `onRecvPacket()` is pure virtual.
- **`Mesh`** (`src/Mesh.h/cpp`): Implements `onRecvPacket()`. Decrypts payloads, manages flood-dedup via `MeshTables`, selects routing strategy, and exposes virtual `on*Recv()` hooks for upper layers.
- **`BaseChatMesh`** (`src/helpers/BaseChatMesh.h/cpp`): Manages a static array of `ContactInfo` contacts and `ChannelDetails` group channels. All memory is statically allocated (no heap after `begin()`).

### Packet structure

A `Packet` (`src/Packet.h`) has:
- `header`: encodes route type (flood/direct/transport), payload type, and protocol version in a single byte.
- `payload[]` (max 184 bytes): encrypted content.
- `path[]` (max 64 bytes): hop hashes accumulated during flood routing, or the supplied path for direct routing.
- `transport_codes[2]`: optional zone/filter codes attached to transport-mode packets.

Payload types are defined as `PAYLOAD_TYPE_*` constants. The two routing modes are flood (`ROUTE_TYPE_FLOOD`, `ROUTE_TYPE_TRANSPORT_FLOOD`) and direct (`ROUTE_TYPE_DIRECT`, `ROUTE_TYPE_TRANSPORT_DIRECT`).

### Variant + example composition

Each of the 73 hardware variants lives in `variants/<name>/` and provides:
- `platformio.ini`: inherits from a platform base (`esp32_base`, `nrf52_base`, `rp2040_base`, `stm32_base`) and sets radio pin mappings and feature flags via `-D` defines.
- `target.h` / `target.cpp`: instantiate board-specific `MainBoard`, radio driver, RTC, and display objects.
- A board class (e.g. `HeltecV3Board`) implementing `mesh::MainBoard`.

The root `platformio.ini` uses `extra_configs = variants/*/platformio.ini` to pull all variants in, so every variant's environments are available from the project root.

Example applications (`examples/companion_radio`, `examples/simple_repeater`, etc.) are plain source directories selected by each env's `build_src_filter`.

### Platform abstractions

| Interface | Purpose |
|---|---|
| `mesh::Radio` | `recvRaw()`, `startSendRaw()`, `isSendComplete()` |
| `mesh::PacketManager` | static pool alloc/free + priority outbound queue |
| `mesh::RTCClock` | UNIX-epoch wall clock |
| `mesh::MillisecondClock` | millis() equivalent |
| `mesh::MainBoard` | battery voltage, reboot, sleep, GPIO, OTA |
| `mesh::MeshTables` | seen-packet deduplication table |

RadioLib is used for all LoRa radios via thin wrapper classes in `src/helpers/radiolib/` (e.g. `CustomSX1262Wrapper`).

### Key design constraints (from README/CONTRIBUTING)

- **No dynamic memory allocation after `begin()`** — all arrays are statically sized via `#define MAX_CONTACTS`, `MAX_NEIGHBOURS`, etc., which are set per-env in `build_flags`.
- **Embedded mindset** — avoid abstractions and layers not strictly needed. Think in terms of fixed-size buffers and direct struct access.
- **Coding style**: 2-space indentation, `camelCase` functions/variables, `PascalCase` classes, `ALL_CAPS` defines. Do NOT retroactively reformat existing code.
- **PRs target `dev` branch**, not `main`.
- For impactful/architectural changes, open an Issue first before submitting a PR.

## Debugging

Enable debug output by adding to `build_flags` in the variant's `platformio.ini`:

```
-D MESH_DEBUG=1          ; core mesh routing debug
-D MESH_PACKET_LOGGING=1 ; per-packet TX/RX logging (do NOT enable in companion_radio builds)
-D BRIDGE_DEBUG=1        ; bridge relay debug
-D BLE_DEBUG_LOGGING=1   ; BLE interface debug
-D WIFI_DEBUG_LOGGING=1  ; WiFi interface debug
```
