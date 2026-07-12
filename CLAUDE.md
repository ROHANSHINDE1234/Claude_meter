# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ESP32 + OLED desk display showing live Claude Code usage, inspired by [Clawdmeter](https://github.com/HermannBjorgvin/Clawdmeter) but reimplemented from scratch for simpler hardware on hand (no touchscreen, no BLE, no LVGL). See [README.md](README.md) for full hardware/wiring details and project history.

## Toolchain (non-negotiable)

**arduino-cli only** — not the Arduino IDE GUI, not PlatformIO. VS Code is a text editor only; all compiling/flashing goes through the CLI. Do not suggest migrating to PlatformIO or the Arduino IDE.

- Board FQBN: `esp32:esp32:esp32` ("ESP32 Dev Module")
- ESP32 core: `esp32:esp32@3.3.10`
- Toolchain/cores/libraries live in `%LOCALAPPDATA%\Arduino15\` (not vendored into the repo)

## Commands

Run from inside `oled_test/` (or pass the sketch folder path):

| Task | Command |
|---|---|
| Check port | `arduino-cli board list` |
| Compile | `arduino-cli compile --fqbn esp32:esp32:esp32 oled_test` |
| Upload | `arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 oled_test` |
| Regenerate IntelliSense data | `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path build --only-compilation-database .` (run from inside `oled_test/`) |
| Install a new library | `arduino-cli lib install "Library Name"` |

There is no automated test suite — validation is "compiles, flashes, and the OLED shows the right thing."

## Architecture decisions to respect

- **Display driver is SH1106, not SSD1306.** The board is a visually-identical-looking 1.3" OLED but is confirmed SH1106 (printed "1.30'' IIC V2.2"). Always use `Adafruit_SH110X` / `Adafruit_SH1106G` / `SH110X_WHITE`, never `Adafruit_SSD1306`. `Adafruit SSD1306` is installed as a library but must not be used — it's a leftover from before the driver mismatch was diagnosed.
- **Transport is USB Serial, not WiFi/MQTT/BLE.** This was a deliberate decision after evaluating alternatives (see README section 3.1) — the ESP32 never touches the internet; only a laptop-side daemon will call the Anthropic API and write JSON down the serial port. Don't reintroduce WiFi/networking code on the ESP32 side.
- **The push button (GPIO27, `INPUT_PULLUP`) is a manual force-refresh trigger**, not a WiFi-reconnect button (that was its original, now-abandoned purpose).
- VS Code's red "cannot open source file" squiggles in `oled_test.ino` are an IntelliSense-only issue (separate include-path system from the real `arduino-cli` compiler) and don't indicate real compile failures — see README section 5 before "fixing" real code over this.

## Project status

The project is at the stage of a validated toolchain + working "Hello ESP32!" display. Not yet implemented: button test, serial JSON receive/parse on the ESP32 side, the laptop-side Python daemon, and full integration testing. See README section 4 (Progress Log) and section 7 (Next Steps) for the current state and planned order of work — check there before assuming what's already built.
