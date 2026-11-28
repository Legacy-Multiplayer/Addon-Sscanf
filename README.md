# Addon-Sscanf

Typed string splitting for Legacy Server, in the spirit of the classic SA-MP sscanf plugin, built on the shared [Addon-SDK](https://github.com/Legacy-Multiplayer/Addon-SDK). Two native Lua functions, no Lua adapter.

```lua
registerCommand("givemoney", function(playerId, params)
    local ok, target, amount = sscanf(params, "ud")
    if not ok or not target then
        return notify(playerId, "Usage", "/givemoney [player] [amount]", "warn")
    end
    givePlayerMoney(target, amount)
end)
```

- `sscanf(input, format)` returns `true, values...` or `false, error, specifierNumber`.
- `sscanfTable(input, format)` returns `{n = count, ...}` or `nil, error, specifierNumber`.

Specifiers: `i d h x o b f g c l w s u r q`, uppercase for optional with `(default)`, `s[n]` for length, `p<c>` delimiters and `'literal'` text. See [docs/API.md](docs/API.md) for the full reference, [docs/BUILDING.md](docs/BUILDING.md) to compile, and [examples/commands.lua](examples/commands.lua) for command patterns.

Parsing runs synchronously on the server thread, with bounded input and format sizes. It does not start background work.

## Get the source

```sh
git clone --recurse-submodules https://github.com/Legacy-Multiplayer/Addon-Sscanf.git
cd Addon-Sscanf
```

For an existing checkout, run `git submodule update --init --recursive`. The SDK is pinned as a Git submodule; rebuild against the exact SDK revision used by your server. GitHub source ZIP downloads do not include submodule contents.

## GitHub builds and releases

Pushes to `main` and pull requests build Windows x86 and Linux x86, run offline tests, and upload packages under **Actions → Build plugins → Artifacts**. Packages include the library, documentation, source/SDK revision manifest and a SHA-256 checksum. Windows produces `.dll`; Linux produces `.so`. A successful compilation does not prove live server behavior.

To prepare a release, manually run **Build plugins** on `main` with a new tag such as `v1.0.0`. Both builds must pass before a **draft** can be created. Nothing publishes automatically.

## Lua Integration and LMP

Release packages include generated `.d.lua` signatures for autocomplete and argument hints.
Definitions are editor-only; do not execute them as server scripts.

```text
lmp install Legacy-Multiplayer/Addon-Sscanf:<version>
```

LMP installs the native addon in `addons/` and its signatures in `.lmp/definitions/`.
Use `--platform linux` or `--platform both` when preparing another target.

For source builds, install the signature generator before configuring CMake:

```text
python -m pip install -r SDK/tools/requirements.txt
```

On newer Visual Studio toolsets, set `LIBCLANG_FILE` to a compatible LLVM `libclang.dll`
when the bundled libclang does not support those headers. `SDK_GENERATE_LUA=OFF` is
for binary-only local builds; releases require generated signatures.

For manual installation, copy the generated `.d.lua` beside the DLL/SO in
`addons/` to enable Lua Integration hints. It is an editor definition, not a
server script.
