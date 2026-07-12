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
- **Transport is direct WiFi + HTTPS, not USB Serial.** Superseded from the original serial-only decision — see `token-usage-feature-decision.md` for the full trade-off discussion. The ESP32 joins WiFi directly and calls `api.anthropic.com` itself (no laptop, no daemon, no relay); it reads live session usage off the response **headers** of a real (but `max_tokens: 1`) inference call, since there's no dedicated usage endpoint. `WiFiConnection` owns getting onto the network (connect + NTP); `ClaudeSessionProvider` owns the actual HTTPS call and just checks `WiFi.status()` first — these are the only two modules that should touch WiFi. The model id used for that call (`claude-haiku-4-5` in `ClaudeSessionProvider.cpp`) is arbitrary — the response body is discarded — but it must stay a **currently-served** model; a retired id 404s (this already happened once with `claude-3-5-haiku-20241022`, retired 2026-02-19). If usage stops working, check that model id against `shared/models.md`-equivalent current docs before assuming anything else is broken.
- **`TokenUsageManager::tick()` must pace retries unconditionally on `lastFetchMs_`, not on last-success.** It previously only backed off after a *successful* fetch, so a persistent failure (e.g. the retired-model 404 above) turned every `loop()` iteration into an API call. `forceRefresh()` stamps `lastFetchMs_` on every attempt regardless of outcome — `tick()` relies on that to pace at `POLL_INTERVAL_MS` even while failing.
- **Credentials live in `claude_meter/include/secrets.h`, which is gitignored — never hardcode WiFi/OAuth values directly in a tracked `.ino`/`.h`/`.cpp` file.** `secrets.example.h` is the tracked template; copy it to `secrets.h` and fill in real values there. The Claude OAuth `accessToken` is short-lived and manually re-pasted when it expires (Approach A in `token-usage-feature-decision.md`) — the ESP32 deliberately does **not** implement its own token refresh, because racing the Claude Code CLI's own refresh can invalidate your actual CLI login. **This file has moved twice already as the project restructured** (root → `src/` → `include/`) — if you move it again, immediately verify `git check-ignore -v` still matches before doing anything else; `.gitignore`'s pattern is a literal path, not a basename glob, so a move silently un-ignores it.
- **The access token now lives in flash (NVS via `TokenStore`), not just the `secrets.h` macro.** `CLAUDE_ACCESS_TOKEN` only seeds `TokenStore` on first boot (empty NVS); after that, updates come from `TokenWebServer`'s page at `http://<esp32-ip>/` — no reflash needed. `ClaudeSessionProvider` reads the token via `tokenStore_.get()`, never the macro directly. This exists because the device is used away from the machine that can mint fresh tokens (see `token-usage-feature-decision.md` §6.1 discussion / conversation history) — don't revert `ClaudeSessionProvider` back to reading `CLAUDE_ACCESS_TOKEN` directly.
- **Submitting a token via the web UI triggers an immediate verification fetch.** `TokenWebServer` holds a `TokenUsageManager &` and calls `forceRefresh()` right after `TokenStore::set()` in `handleUpdate()`, then reports success/failure in the HTTP response based on `usageManager_.current().valid`. Without this, a good submit would still leave the QR on screen for up to `POLL_INTERVAL_MS`. Don't strip this out to "simplify" `TokenWebServer` back to just `TokenStore` — it closes a real UX gap.
- **`TokenWebServer`'s password gate is a shared-secret deterrent, not real security** — plain HTTP, no TLS, reachable by anything on the same WiFi/hotspot. Don't extend it to control anything more sensitive without adding real auth first.
- **The boot screen shows a QR code linking to the token-update page, not a fixed-duration text screen.** `QRCodeScreen` (implements `IScreen`, same interface as `TokenUsageScreen`) uses ESP-IDF's bundled `qrcode` component (`espressif__qrcode`, header `qrcode.h`) — **not** a separately-installed Arduino library. A separate `QRCode` library (ricmoo) was tried first and removed: the ESP32 core's own `qrcode.h` is earlier on the include path and silently wins the collision, so installing another library of the same header name doesn't work regardless of what's in `libraries/`. `esp_qrcode_generate()`'s `display_func` callback has no context pointer, hence the static-member/scratch-state pattern inside `QRCodeScreen` — this is deliberate, not an oversight to "clean up." It draws once (`rendered_` flag) since the image is static, not on every periodic tick.
- **The QR code stays on screen until the first successful usage fetch, not for a fixed duration.** `claude_meter.ino` tracks `waitingForFirstFetch`; once `usageManager.current().valid` is true, it calls `displayManager.setScreen(&usageScreen)` to hand off. `DisplayManager::tick()` always runs unconditionally now — the handoff is a screen swap, not a "skip rendering" branch. A bad/expired token leaves the QR showing indefinitely — intentional, since that's exactly when scanning it to paste a new token matters.
- **Code layout is `include/` (headers) + `src/` (implementation), and this split is load-bearing, not cosmetic.** Confirmed empirically against this exact toolchain: Arduino's sketch build only auto-compiles a subfolder literally named `src/` — any other name is silently skipped (no linker error trail, functions just never get compiled). Bare `#include "Foo.h"` only resolves within the same folder — `claude_meter.ino` uses `#include "include/Foo.h"`, every `.cpp` in `src/` uses `#include "../include/Foo.h"` for its own header and any others it needs, and headers within `include/` bare-include each other freely. **Do not add a new implementation file to any folder other than `src/`** (or a subfolder under it) — it will compile with zero errors and then fail at *link* time with "undefined reference," which is a confusing failure mode if you don't already know this rule. See README section 3.5 and 5 for the full detail and the exact test that confirmed it.
- **TLS certificate validation is intentionally skipped for now** (`WiFiClientSecure::setInsecure()` in `ClaudeSessionProvider.cpp`) — acceptable for a prototype on a trusted home network per the decision doc, but worth revisiting before this leaves the breadboard stage.
- **The push button (GPIO27, `INPUT_PULLUP`) is a manual force-refresh trigger**, not a WiFi-reconnect button (that was its original, now-abandoned purpose).
- VS Code's red "cannot open source file" squiggles in `.ino` files are an IntelliSense-only issue (separate include-path system from the real `arduino-cli` compiler) and don't indicate real compile failures — see README section 5 before "fixing" real code over this.

## Project status

`oled_test/` (validated toolchain + "Hello ESP32!") is done. `claude_meter/` is **validated end-to-end on real hardware at home**, including the full token-update loop: WiFi connects, the live Anthropic header-scraping call returns 200 with real rate-limit headers, NTP sync succeeds, the OLED displays correct live data, and the QR-code-to-web-UI token update flow (scan → open page → paste token → immediate verification → usage screen) has been run successfully at least once. Several real bugs surfaced during hardware bring-up and are fixed — see the bullets above on model id, `tick()` pacing, and the code-layout/include-path rules discovered while restructuring.

**What's untested is the specific office scenario**, not the underlying mechanisms: booting on an office mobile hotspot with an overnight-expired token, scanning the QR from a phone on that hotspot (not the office PC — different network), pulling a fresh token from the office PC's `.claude/.credentials.json`, and confirming the whole loop closes in that actual environment. Also still unexercised: the push button's force-refresh, and an extended real-world run. See `token-usage-feature-decision.md` for the design rationale and open decisions, and README section 4 (Progress Log) / section 7 (Next Steps) for what's next.
