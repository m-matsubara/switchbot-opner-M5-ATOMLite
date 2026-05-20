# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

M5 ATOM Lite (ESP32) firmware that controls a SwitchBot SmartLock Pro via the SwitchBot Cloud REST API. The device connects to WiFi, authenticates with HMAC-SHA256 signatures, and sends lock/unlock commands over HTTPS. Status is shown via a single RGB LED (no LCD). This is an adaptation of the M5StickC Plus version at `C:\data\develop\switchbot-opener`.

## Build and Upload

```bash
pio run                # Build
pio run -t upload      # Flash to device
pio device monitor     # Serial monitor (115200 baud)
```

PlatformIO environment: `m5atom_lite` (board: `m5stack-atom`, platform: `espressif32`, framework: Arduino).

## Source Structure

- `src/main.cpp` — entire application (WiFi, NTP, HMAC-SHA256 auth, HTTPS API calls, button handling, LED control)
- `src/const.hpp.example` — template for credentials; copy to `src/const.hpp` and fill in WiFi/SwitchBot values
- `src/const.hpp` — gitignored, contains actual credentials

## Hardware Details

- Single button on GPIO39 (active-low): short press = unlock, long press (2s) = lock
- Single RGB LED via `M5.dis.drawpix(0, CRGB(r, g, b))` — normally off; blinks on success, solid color during transient states
- No battery/AXP192 — USB-powered only, no auto power-off

## API Authentication Flow

SwitchBot API v1.1 requires these headers: `Authorization` (token), `sign` (HMAC-SHA256 of `token + timestamp_ms + nonce`, Base64 uppercase), `t` (timestamp ms), `nonce` (UUID v4). The signature is computed using mbedtls.

## Coding Conventions

- 2-space indentation, UTF-8, LF line endings (see `.editorconfig`)
- All state and helpers are file-static in `main.cpp`
- Japanese documentation in README.md
