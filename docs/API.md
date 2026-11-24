# sscanf functions

Load the native Addon-Sscanf plugin by placing `Addon-Sscanf.dll` (or `.so`) in the server's `addons` folder. These are global Lua functions; no Lua file is required.

## sscanf(input, format)

Splits `input` into typed values according to `format`. `input` may be `nil` (treated as empty text), which is convenient for command parameters.

Return on success: `true, value1, value2, ...` (at most 15 values; use `sscanfTable` for more).
Return on failure: `false, errorMessage, specifierNumber`.

```lua
registerCommand("pay", function(playerId, params)
    local ok, target, amount = sscanf(params, "ud")
    if not ok then
        return notify(playerId, "Usage", "/pay [player] [amount]", "warn")
    end
    if not target then
        return notify(playerId, "Player not found", nil, "warn")
    end
    givePlayerMoney(target, amount)
end)
```

Malformed formats (unknown specifier, unclosed literal, default value on a required specifier) raise a Lua error, because they are script bugs rather than bad player input.

## sscanfTable(input, format)

Same rules, but returns the values in one table so there is no 15-value limit.

Return on success: `values, nil, nil` where `values` is `{n = count, ...}` like `table.pack`.
Return on failure: `nil, errorMessage, specifierNumber`.

```lua
local values, problem = sscanfTable("1 2 3 4 5", "ddddd")
print(values.n, values[1], values[5])
```

## Specifiers

| Specifier | Reads | Lua value |
| --- | --- | --- |
| `i` | Integer; accepts `0x` hex and `0b` binary prefixes | integer |
| `d` | Decimal integer | integer |
| `h`, `x` | Hexadecimal integer, optional `0x` | integer |
| `o` | Octal integer | integer |
| `b` | Binary integer, optional `0b` | integer |
| `f`, `g` | Decimal number | number |
| `c` | Exactly one character | string |
| `l` | Logical: `1/0`, `true/false`, `yes/no`, `y/n`, `on/off` | boolean |
| `w` | One word | string |
| `s` | One word, or **the rest of the input** when it is the last specifier | string |
| `s[n]` | Same as `s`, truncated to `n` bytes | string |
| `u` | Player or NPC by ID or name | player ID, or `nil` if no unique match |
| `r` | Human player only | player ID or `nil` |
| `q` | NPC only | player ID or `nil` |

Integers are signed 64-bit and reject overflow. Numbers must be finite.

### Players

`u`, `r` and `q` accept a numeric ID (must be connected) or a name. Names match case-insensitively and treat `_` and space as equal. An exact name wins, then a unique prefix, then a unique substring. Ambiguous or unknown names give `nil`; the scan still succeeds, so check the value.

### Optional values

An uppercase specifier is optional: `I`, `D`, `F`, `S`, `U` and so on. Add a default in parentheses: `I(10)`, `S(none)`, `F(1.5)`, `U(-1)`. Without a default a missing optional value is `nil`. `z` is the same as `S`. A player default that is a negative number gives `nil`.

If an optional value is present but invalid, the scan fails, exactly like a required value.

### Delimiters and literals

`p<c>` changes the field delimiter for the specifiers after it, for example `p<,>dsf` reads `12, Kero Konbini, 4.5`. Spaces around each field are trimmed and an empty field counts as missing.

`'text'` requires that literal text (case-insensitive) at that position, for example `'give' u 'money' d`.

## Limits

Input up to 4096 bytes, format up to 512 bytes, and up to 64 values per scan.
