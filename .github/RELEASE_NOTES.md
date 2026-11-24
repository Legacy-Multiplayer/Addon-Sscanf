# Addon-Sscanf v1.0.0

Typed command-parameter parsing for Legacy Multiplayer servers, with native Lua functions and player/NPC lookup.

## Downloads

Windows x86 `.dll` and Linux x86 `.so` packages include documentation, a source/SDK revision manifest and SHA-256 checksums. The addon must match the server SDK fingerprint. Windows requires the x86 MSVC runtime; Linux uses an Ubuntu 22.04 x86 build baseline with compatible 32-bit runtimes.

## Functions

- `sscanf(input, format)` returns typed values or structured parse failure.
- `sscanfTable(input, format)` returns a counted value table.
- Supports numeric, string, delimiter, literal, optional and player/NPC parameters.

Parsing is synchronous and bounded. Verify Lua command behavior and player/NPC resolution in your server before production use.

See the repository README and docs for installation, API details and validation limits. Clone recursively when building from source.
