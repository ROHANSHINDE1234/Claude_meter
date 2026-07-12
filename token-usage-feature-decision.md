# Token Usage Feature — Architecture & Decision Document

**Status:** Design discussed, no code written yet. This document exists so you can think through the trade-offs before committing to an approach.

---

## 1. Current Project State (going into this discussion)

- Breadboard prototype fully working: ESP32-WROOM-32 DevKit + SH1106 1.3" OLED, wired and confirmed via "Hello ESP32!"
- Toolchain: `arduino-cli`, no vendor IDE
- You are currently **at home, with WiFi access**, and the device already connects to your home network successfully
- Goal for this next feature: show live Claude usage on the OLED

---

## 2. Proposed Architecture (agreed in principle)

A layered, modular design so future screens (WiFi status, battery, latency, etc.) can be added without touching existing code:

```
[Usage Provider] → [Parser] → [TokenUsageManager] → [Display Manager] → [OLED]
```

| Module | Responsibility |
|---|---|
| `hw_config.h` | All pin numbers / constants — single source of hardware facts |
| `IUsageProvider` (interface) | "Give me a fresh usage reading" — implementation is swappable |
| `ClaudeSessionProvider` | The concrete implementation discussed below — makes the actual network call |
| `json_parser` / header parser | Turns a raw response into a fixed `UsageData` struct |
| `TokenUsageManager` | Owns current usage state + staleness tracking; the single source of truth |
| `IScreen` (interface) | One `render()` method; makes adding new screens (WiFi status, battery) trivial later |
| `TokenUsageScreen` | First concrete screen, renders whatever `TokenUsageManager` holds |
| `DisplayManager` | Owns the OLED object + which screen is currently active |
| `claude_meter.ino` | Orchestration only — init each module, call each module's non-blocking `tick()` |

This part of the design isn't in question — it holds regardless of how data actually gets fetched. What's still open is the content of `ClaudeSessionProvider` itself, which is what the rest of this document is about.

---

## 3. The Data Source Question

### 3.1 What "usage" actually means here

You want the **real session usage percentage and time-until-reset** — the same metric Clawdmeter displays, and the same one `claude.ai`'s own usage page shows. This is *not* the same as "input/output token counts from a single API call" — it's your account's actual 5-hour and 7-day rate-limit utilization.

### 3.2 How this data is actually obtainable (verified against how Clawdmeter and similar community tools do it)

There is no dedicated "give me my usage" endpoint that works reliably — Anthropic's own `/api/oauth/usage` endpoint exists but is currently known to be aggressively rate-limited to the point of being unusable (documented, open issue).

The method that actually works: **piggyback on a real, tiny inference call.**

- `POST https://api.anthropic.com/v1/messages`, with `max_tokens: 1`, cheapest available model
- Cost: a fraction of a cent per poll
- Required request headers alongside your bearer token:
  - `anthropic-beta: oauth-2025-04-20`
  - A `User-Agent` resembling a Claude Code client (omitting this has been reported to trigger aggressive rate-limiting)
- The response **headers** (not the response body) carry the actual numbers:
  - `anthropic-ratelimit-unified-5h-utilization` — session usage, fraction 0–1
  - `anthropic-ratelimit-unified-5h-reset` — reset time, Unix timestamp
  - `anthropic-ratelimit-unified-7d-utilization` / `-7d-reset` — weekly equivalents, available if you want them later

### 3.3 Where the credential comes from

Your Claude Code CLI already has a valid OAuth token sitting in:
- Linux: `~/.claude/.credentials.json`
- Windows: `%USERPROFILE%\.claude\.credentials.json`
- macOS: stored in Keychain, not a flat file

The relevant field is the `accessToken` inside that file (often nested under a `claudeAiOauth` key).

---

## 4. The Real Risk: Token Lifetime and Refresh

This is the part most worth sitting with before deciding.

- The `accessToken` is **short-lived**. Once it expires, calls will start failing with 401 until a fresh token is supplied.
- It's paired with a `refreshToken` — but that refresh token is **single-use and rotates** every time it's used.
- **Critical danger:** if the ESP32 tries to refresh this token independently, and your Claude Code CLI *also* refreshes it around the same time (which it will, since you'll keep using Claude Code normally on this machine) — whichever process loses that race gets `invalid_grant`. This doesn't just break the display: it can invalidate your actual Claude Code login, requiring a manual `claude login` to fix.
- This is a **documented failure mode** other people building similar tools have hit, not a hypothetical.

### 4.1 Two ways to handle this

| Approach | What it means |
|---|---|
| **A — No refresh logic on ESP32 (recommended starting point)** | Hardcode the current `accessToken` as-is. It works until it naturally expires, then you manually copy a fresh one in. Zero risk to your actual Claude Code login. Matches exactly what you described wanting for the first iteration ("display it once"). |
| **B — ESP32 implements its own refresh** | More "set and forget," but carries the rotation-race risk above. Some existing tools sidestep this by explicitly disabling their own refresh and only ever reading whatever token the CLI itself most recently wrote — worth considering *if* you want auto-refresh later, but adds real complexity and a dependency on file-level access to the CLI's credentials store, which the ESP32 doesn't have anyway (it's not running on your laptop). |

Given the ESP32 has no way to read your laptop's credentials file directly regardless, "B" would actually require a laptop-side companion process rewriting the hardcoded value somehow — which reopens the daemon-relay design we set aside for the mobile-hotspot plan. Worth noting as a real fork, not just a footnote.

---

## 5. How the ESP32 Actually Reaches Anthropic (your direct question)

```
ESP32 → joins your home WiFi → router → internet → api.anthropic.com
```

This is simpler than the earlier mobile-hotspot design specifically because `api.anthropic.com` is a **fixed public hostname** — not something living on your local network whose address changes. Your home WiFi already has a real route to the internet, so:

1. ESP32 connects to your home WiFi (already working)
2. Makes a direct outbound HTTPS request to Anthropic's servers — no laptop, no daemon, no relay involved
3. Anthropic responds; the ESP32 reads the headers itself

One implementation detail this adds: since it's HTTPS, the firmware needs `WiFiClientSecure` (TLS) rather than plain `WiFiClient`. For a first prototype, `setInsecure()` (skip certificate validation) is the simplest path — acceptable for a device on your own trusted network, though it means no verification you're really talking to Anthropic's real server. Proper certificate validation can be added later without changing anything else in the design.

---

## 6. Open Decisions for You to Think Through

1. **Are you comfortable with your Claude Code OAuth access token living in ESP32 flash** — on a breadboard, on your desk? (Different risk profile than a scoped, revocable API key — this token is tied to your actual account session.)
2. **Approach A or B from section 4** — manual token refresh (simple, safe, some upkeep) vs. attempting auto-refresh (convenient, real risk of locking yourself out of Claude Code CLI until re-login)?
3. **Do you want TLS certificate validation from the start**, or is `setInsecure()` acceptable for this prototype phase?
4. **Confirm the displayed metric**: session utilization % + reset countdown (and optionally the weekly 7-day equivalent) — not raw input/output token counts, since those aren't what this method provides.

---

## 7. My Recommendation (for when you're ready to decide)

Start with **Approach A** (section 4.1) — no refresh logic, manually re-paste the token when it expires. It gets you the real feature working fastest, carries zero risk to your actual Claude Code login, and matches what you originally described wanting for this first pass. Revisit auto-refresh later only if the manual re-paste cadence turns out to be annoying in practice — and if so, it's worth treating as a deliberate second design discussion rather than folding it in now.
