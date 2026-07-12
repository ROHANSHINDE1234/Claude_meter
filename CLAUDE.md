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

There are two sketches. `oled_test/` is the original bring-up sketch (kept as a known-good fallback). `claude_meter/` is the real product — the modular token-usage display described below. Run commands from inside the relevant sketch folder (or pass its path):

| Task | Command |
|---|---|
| Check port | `arduino-cli board list` |
| Compile | `arduino-cli compile --fqbn esp32:esp32:esp32 claude_meter` |
| Upload | `arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 claude_meter` |
| Regenerate IntelliSense data | `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path build --only-compilation-database .` (run from inside the sketch folder) |
| Install a new library | `arduino-cli lib install "Library Name"` |

There is no automated test suite — validation is "compiles, flashes, and the OLED shows the right thing."

## Architecture decisions to respect

- **Display driver is SH1106, not SSD1306.** The board is a visually-identical-looking 1.3" OLED but is confirmed SH1106 (printed "1.30'' IIC V2.2"). Always use `Adafruit_SH110X` / `Adafruit_SH1106G` / `SH110X_WHITE`, never `Adafruit_SSD1306`. `Adafruit SSD1306` is installed as a library but must not be used — it's a leftover from before the driver mismatch was diagnosed.
- **Transport is direct WiFi + HTTPS, not USB Serial.** Superseded from the original serial-only decision — see `token-usage-feature-decision.md` for the full trade-off discussion. The ESP32 joins WiFi directly and calls `api.anthropic.com` itself (no laptop, no daemon, no relay); it reads live session usage off the response **headers** of a real (but `max_tokens: 1`) inference call, since there's no dedicated usage endpoint. `ClaudeSessionProvider` is the only module that should touch WiFi/HTTPS. The model id used for that call (`claude-haiku-4-5` in `ClaudeSessionProvider.cpp`) is arbitrary — the response body is discarded — but it must stay a **currently-served** model; a retired id 404s (this already happened once with `claude-3-5-haiku-20241022`, retired 2026-02-19). If usage stops working, check that model id against `shared/models.md`-equivalent current docs before assuming anything else is broken.
- **`TokenUsageManager::tick()` must pace retries unconditionally on `lastFetchMs_`, not on last-success.** It previously only backed off after a *successful* fetch, so a persistent failure (e.g. the retired-model 404 above) turned every `loop()` iteration into an API call. `forceRefresh()` stamps `lastFetchMs_` on every attempt regardless of outcome — `tick()` relies on that to pace at `POLL_INTERVAL_MS` even while failing.
- **Credentials live in `claude_meter/secrets.h`, which is gitignored — never hardcode WiFi/OAuth values directly in a tracked `.ino`/`.h`/`.cpp` file.** `secrets.example.h` is the tracked template; copy it to `secrets.h` and fill in real values there. The Claude OAuth `accessToken` is short-lived and manually re-pasted when it expires (Approach A in `token-usage-feature-decision.md`) — the ESP32 deliberately does **not** implement its own token refresh, because racing the Claude Code CLI's own refresh can invalidate your actual CLI login.
- **TLS certificate validation is intentionally skipped for now** (`WiFiClientSecure::setInsecure()` in `ClaudeSessionProvider.cpp`) — acceptable for a prototype on a trusted home network per the decision doc, but worth revisiting before this leaves the breadboard stage.
- **The push button (GPIO27, `INPUT_PULLUP`) is a manual force-refresh trigger**, not a WiFi-reconnect button (that was its original, now-abandoned purpose).
- VS Code's red "cannot open source file" squiggles in `.ino` files are an IntelliSense-only issue (separate include-path system from the real `arduino-cli` compiler) and don't indicate real compile failures — see README section 5 before "fixing" real code over this.

## Project status

`oled_test/` (validated toolchain + "Hello ESP32!") is done. `claude_meter/` is **validated end-to-end on real hardware** — WiFi connects, the live Anthropic header-scraping call returns 200 with real rate-limit headers, NTP sync succeeds, and the OLED displays correct live data (confirmed via photo: `Claude usage / 37% / Resets in 2h 09m`). Two real bugs surfaced during hardware bring-up and are fixed (see the two bullets above on model id and `tick()` pacing) — this is why Serial diagnostics exist in `claude_meter.ino` / `ClaudeSessionProvider.cpp` now; keep them rather than stripping them out, they're the only visibility into WiFi/HTTP failures. Remaining work is the push button's force-refresh (code exists, not yet specifically exercised) and an extended real-world run. See `token-usage-feature-decision.md` for the design rationale and open decisions, and README section 4 (Progress Log) / section 7 (Next Steps) for what's next.
