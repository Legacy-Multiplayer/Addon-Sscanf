#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Scanner
{
enum class Kind
{
    Nil,
    Integer,
    Number,
    String,
    Boolean
};

struct Value
{
    Kind kind = Kind::Nil;
    int64_t integer = 0;
    double number = 0.0;
    bool boolean = false;
    std::string text;

    static Value Nil();

    static Value Integer(int64_t value);

    static Value Number(double value);

    static Value Boolean(bool value);

    static Value String(std::string value);
};

enum class PlayerFilter
{
    Any,
    Human,
    Bot
};

using PlayerResolver = std::function<std::optional<int64_t>(const std::string& query, PlayerFilter filter)>;

struct Result
{
    bool success = false;
    std::vector<Value> values;
    std::string error;
    size_t specifier = 0;
};

constexpr size_t MaximumInput = 4096;
constexpr size_t MaximumFormat = 512;
constexpr size_t MaximumValues = 64;

Result Scan(std::string_view input, std::string_view format, const PlayerResolver& resolver);

} // namespace Scanner
