# Validation status

Source and local build review: 24 November 2026. The checks below were completed before the initial push. No server installation or release publication is part of this review.

## Confirmed locally

- Windows x86 Release build succeeded.
- The actual DLL loads in the x86 test process. All four exports exist; API version and SDK fingerprint match the pinned SDK. Null initialization is rejected safely.
- Windows package passes PE/x86/DLL and export checks. SHA-256 checksums and a source/SDK manifest are generated. Local validation packages are explicitly marked dirty/uncommitted and must not be published.
- SDK is a Git submodule (mode `160000`) pinned to `e506d62f904567aa574c37796c072167f7513f23`. Compared against the local server SDK at `d84c2d2d259cf5a7db60ccd2966cd488ee5034ed`: only README.md differs; all SDK code is identical.
- Workflow YAML structure and packaging Python syntax were checked locally. GitHub Actions has not run for these new repositories yet.
- Public-source scan found no personal machine paths or token-shaped credentials. Local `discord.cfg` and `.env` files are ignored. This is a targeted scan, not proof that arbitrary future files contain no secrets.

- CTest: PluginExports and ScannerTests passed (2/2). ScannerTests reports 67 checks, zero failures.
- Parser coverage includes numeric bounds, optional/default arguments, delimiters, strings, literals, malformed input and fake player resolution.

## Corrections made during review

- Renamed the directory, CMake target, plugin identity and documentation consistently to Addon-Sscanf.
- Removed the redundant hardcoded 1,024-ID fallback from player-name lookup. The SDK enumerators cover current pool IDs and separate human/NPC types using server generation checks.
- Clarified that `s[n]` truncates bytes, not Unicode characters, and removed an unmeasured microsecond performance claim.

## Still required before publication

- Linux x86 build/test/package execution in GitHub Actions (no usable Linux runtime was available locally).
- Live Legacy Server registration of both Lua functions, raw-return behavior, table-return behavior and parse errors.
- Real human/NPC lookup by ID and name, including IDs above 1,023, ambiguous names, disconnects and ID reuse.
- Verify the exact server executable uses the matching SDK fingerprint. Parser fixtures and a standalone DLL load do not prove the complete live Lua/host integration.

No known source-level release blocker was found in the reviewed parser. This does not certify all gameplay or server configurations.
