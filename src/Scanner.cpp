#include "Scanner.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace Scanner
{
Value Value::Nil()
{
    return {};
}

Value Value::Integer(int64_t value)
{
    Value result;

    result.kind = Kind::Integer;
    result.integer = value;

    return result;
}

Value Value::Number(double value)
{
    Value result;

    result.kind = Kind::Number;
    result.number = value;

    return result;
}

Value Value::Boolean(bool value)
{
    Value result;

    result.kind = Kind::Boolean;
    result.boolean = value;

    return result;
}

Value Value::String(std::string value)
{
    Value result;

    result.kind = Kind::String;
    result.text = std::move(value);

    return result;
}

namespace
{
struct Failure
{
    std::string message;
};

bool IsSpace(char character)
{
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

char Lower(char character)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

std::string Trim(std::string_view text)
{
    size_t begin = 0;
    size_t end = text.size();

    while (begin < end && IsSpace(text[begin]))
    {
        ++begin;
    }

    while (end > begin && IsSpace(text[end - 1]))
    {
        --end;
    }

    return std::string(text.substr(begin, end - begin));
}

std::string ToLower(std::string_view text)
{
    std::string result(text);

    for (auto& character : result)
    {
        character = Lower(character);
    }

    return result;
}

struct Specifier
{
    char type = 0;
    bool optional = false;
    std::optional<std::string> fallback;
    size_t length = 0;
    std::string literal;
    bool isLiteral = false;
    char delimiter = 0;
    bool isDelimiter = false;
};

std::vector<Specifier> ParseFormat(std::string_view format)
{
    std::vector<Specifier> specifiers;
    size_t position = 0;

    while (position < format.size())
    {
        const char character = format[position];

        if (IsSpace(character))
        {
            ++position;
            continue;
        }

        if (character == '\'')
        {
            const size_t close = format.find('\'', position + 1);

            if (close == std::string_view::npos)
            {
                throw std::invalid_argument("Unclosed literal in format");
            }

            Specifier literal;
            literal.isLiteral = true;
            literal.literal = std::string(format.substr(position + 1, close - position - 1));

            if (literal.literal.empty())
            {
                throw std::invalid_argument("Empty literal in format");
            }

            specifiers.push_back(std::move(literal));
            position = close + 1;
            continue;
        }

        if (character == 'p' || character == 'P')
        {
            if (format.size() < position + 4 || format[position + 1] != '<' || format[position + 3] != '>')
            {
                throw std::invalid_argument("Delimiter must look like p<,>");
            }

            Specifier delimiter;
            delimiter.isDelimiter = true;
            delimiter.delimiter = format[position + 2];
            specifiers.push_back(delimiter);
            position += 4;
            continue;
        }

        const char lower = Lower(character);
        static const std::string_view known = "idhxobfgclswzurq";

        if (known.find(lower) == std::string_view::npos)
        {
            throw std::invalid_argument(std::string("Unknown format specifier '") + character + "'");
        }

        Specifier specifier;
        specifier.type = lower == 'z' ? 's' : lower;
        specifier.optional = lower == 'z' || std::isupper(static_cast<unsigned char>(character)) != 0;
        ++position;

        if (position < format.size() && format[position] == '(')
        {
            const size_t close = format.find(')', position);

            if (close == std::string_view::npos)
            {
                throw std::invalid_argument("Unclosed default value in format");
            }

            if (!specifier.optional)
            {
                throw std::invalid_argument("Only optional (uppercase) specifiers can have a default value");
            }

            specifier.fallback = std::string(format.substr(position + 1, close - position - 1));
            position = close + 1;
        }

        if (position < format.size() && format[position] == '[')
        {
            const size_t close = format.find(']', position);

            if (close == std::string_view::npos)
            {
                throw std::invalid_argument("Unclosed length in format");
            }

            const auto digits = format.substr(position + 1, close - position - 1);

            if (digits.empty() || digits.size() > 5)
            {
                throw std::invalid_argument("Invalid string length in format");
            }

            for (const char digit : digits)
            {
                if (!std::isdigit(static_cast<unsigned char>(digit)))
                {
                    throw std::invalid_argument("Invalid string length in format");
                }
            }

            specifier.length = static_cast<size_t>(std::stoul(std::string(digits)));

            if (specifier.length == 0)
            {
                throw std::invalid_argument("String length must be at least 1");
            }

            position = close + 1;
        }

        specifiers.push_back(std::move(specifier));
    }

    return specifiers;
}

bool ParseInteger(const std::string& text, int base, int64_t& output)
{
    if (text.empty())
    {
        return false;
    }

    size_t position = 0;
    bool negative = false;

    if (text[position] == '+' || text[position] == '-')
    {
        negative = text[position] == '-';
        ++position;
    }

    if (base == 0)
    {
        base = 10;

        if (text.size() > position + 1 && text[position] == '0' && Lower(text[position + 1]) == 'x')
        {
            base = 16;
            position += 2;
        }
        else if (text.size() > position + 1 && text[position] == '0' && Lower(text[position + 1]) == 'b')
        {
            base = 2;
            position += 2;
        }
    }
    else if (base == 16 && text.size() > position + 1 && text[position] == '0' && Lower(text[position + 1]) == 'x')
    {
        position += 2;
    }
    else if (base == 2 && text.size() > position + 1 && text[position] == '0' && Lower(text[position + 1]) == 'b')
    {
        position += 2;
    }

    if (position >= text.size())
    {
        return false;
    }

    uint64_t value = 0;
    const uint64_t limit = negative ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1 : static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

    for (; position < text.size(); ++position)
    {
        const char character = Lower(text[position]);
        int digit = -1;

        if (character >= '0' && character <= '9')
        {
            digit = character - '0';
        }
        else if (character >= 'a' && character <= 'z')
        {
            digit = character - 'a' + 10;
        }

        if (digit < 0 || digit >= base)
        {
            return false;
        }

        if (value > (limit - static_cast<uint64_t>(digit)) / static_cast<uint64_t>(base))
        {
            return false;
        }

        value = value * static_cast<uint64_t>(base) + static_cast<uint64_t>(digit);
    }

    if (negative)
    {
        output = value == static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1 ? std::numeric_limits<int64_t>::min() : -static_cast<int64_t>(value);
    }
    else
    {
        output = static_cast<int64_t>(value);
    }

    return true;
}

bool ParseNumber(const std::string& text, double& output)
{
    if (text.empty())
    {
        return false;
    }

    for (const char character : text)
    {
        if (!std::isdigit(static_cast<unsigned char>(character)) && character != '.' && character != '-' && character != '+' && Lower(character) != 'e')
        {
            return false;
        }
    }

    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);

    if (!end || *end != '\0' || !std::isfinite(value))
    {
        return false;
    }

    output = value;

    return true;
}

bool ParseLogical(const std::string& text, bool& output)
{
    const auto lower = ToLower(text);

    if (lower == "1" || lower == "true" || lower == "yes" || lower == "y" || lower == "on")
    {
        output = true;
        return true;
    }

    if (lower == "0" || lower == "false" || lower == "no" || lower == "n" || lower == "off")
    {
        output = false;
        return true;
    }

    return false;
}

class Reader
{
public:
    explicit Reader(std::string_view input) : m_input(input)
    {
    }

    void SetDelimiter(char delimiter)
    {
        m_delimiter = delimiter;
    }

    void SkipSpace()
    {
        while (m_position < m_input.size() && IsSpace(m_input[m_position]))
        {
            ++m_position;
        }
    }

    bool AtEnd()
    {
        SkipSpace();
        return m_position >= m_input.size();
    }

    std::optional<std::string> Field(bool rest)
    {
        SkipSpace();

        if (m_position >= m_input.size())
        {
            return std::nullopt;
        }

        if (rest)
        {
            auto text = Trim(m_input.substr(m_position));
            m_position = m_input.size();
            return text;
        }

        size_t end = m_position;

        if (m_delimiter && !IsSpace(m_delimiter))
        {
            while (end < m_input.size() && m_input[end] != m_delimiter)
            {
                ++end;
            }

            auto text = Trim(m_input.substr(m_position, end - m_position));
            m_position = end < m_input.size() ? end + 1 : end;
            return text;
        }

        while (end < m_input.size() && !IsSpace(m_input[end]))
        {
            ++end;
        }

        auto text = std::string(m_input.substr(m_position, end - m_position));
        m_position = end;
        return text;
    }

    bool Literal(const std::string& literal)
    {
        SkipSpace();

        if (m_input.size() - m_position < literal.size())
        {
            return false;
        }

        for (size_t index = 0; index < literal.size(); ++index)
        {
            if (Lower(m_input[m_position + index]) != Lower(literal[index]))
            {
                return false;
            }
        }

        m_position += literal.size();

        if (m_delimiter && !IsSpace(m_delimiter))
        {
            SkipSpace();

            if (m_position < m_input.size() && m_input[m_position] == m_delimiter)
            {
                ++m_position;
            }
        }

        return true;
    }

private:
    std::string_view m_input;
    size_t m_position = 0;
    char m_delimiter = 0;
};

const char* Describe(char type)
{
    switch (type)
    {
    case 'i':
    case 'd':
        return "a whole number";
    case 'h':
    case 'x':
        return "a hexadecimal number";
    case 'o':
        return "an octal number";
    case 'b':
        return "a binary number";
    case 'f':
    case 'g':
        return "a number";
    case 'c':
        return "a single character";
    case 'l':
        return "true or false";
    case 'u':
    case 'r':
    case 'q':
        return "a player name or ID";
    default:
        return "text";
    }
}

Value Convert(const Specifier& specifier, const std::string& text, const PlayerResolver& resolver, bool fromDefault)
{
    int64_t integer = 0;
    double number = 0.0;
    bool logical = false;

    switch (specifier.type)
    {
    case 'i':
        if (ParseInteger(text, 0, integer))
        {
            return Value::Integer(integer);
        }
        break;
    case 'd':
        if (ParseInteger(text, 10, integer))
        {
            return Value::Integer(integer);
        }
        break;
    case 'h':
    case 'x':
        if (ParseInteger(text, 16, integer))
        {
            return Value::Integer(integer);
        }
        break;
    case 'o':
        if (ParseInteger(text, 8, integer))
        {
            return Value::Integer(integer);
        }
        break;
    case 'b':
        if (ParseInteger(text, 2, integer))
        {
            return Value::Integer(integer);
        }
        break;
    case 'f':
    case 'g':
        if (ParseNumber(text, number))
        {
            return Value::Number(number);
        }
        break;
    case 'c':
        if (text.size() == 1)
        {
            return Value::String(text);
        }
        break;
    case 'l':
        if (ParseLogical(text, logical))
        {
            return Value::Boolean(logical);
        }
        break;
    case 's':
    case 'w':
        if (specifier.length && text.size() > specifier.length)
        {
            return Value::String(text.substr(0, specifier.length));
        }
        return Value::String(text);
    case 'u':
    case 'r':
    case 'q':
    {
        if (fromDefault && ParseInteger(text, 10, integer))
        {
            return integer < 0 ? Value::Nil() : Value::Integer(integer);
        }

        const auto filter = specifier.type == 'r' ? PlayerFilter::Human : specifier.type == 'q' ? PlayerFilter::Bot : PlayerFilter::Any;
        const auto player = resolver ? resolver(text, filter) : std::nullopt;

        return player ? Value::Integer(*player) : Value::Nil();
    }
    default:
        break;
    }

    throw Failure{std::string("Expected ") + Describe(specifier.type) + ", got \"" + text + "\""};
}

bool TakesRest(const std::vector<Specifier>& specifiers, size_t index)
{
    if (specifiers[index].type != 's')
    {
        return false;
    }

    for (size_t next = index + 1; next < specifiers.size(); ++next)
    {
        if (!specifiers[next].isDelimiter)
        {
            return false;
        }
    }

    return true;
}

} // namespace

Result Scan(std::string_view input, std::string_view format, const PlayerResolver& resolver)
{
    if (input.size() > MaximumInput)
    {
        throw std::length_error("Input exceeds 4096 bytes");
    }

    if (format.size() > MaximumFormat)
    {
        throw std::length_error("Format exceeds 512 bytes");
    }

    const auto specifiers = ParseFormat(format);
    Result result;
    Reader reader(input);
    size_t number = 0;

    for (size_t index = 0; index < specifiers.size(); ++index)
    {
        const auto& specifier = specifiers[index];

        if (specifier.isDelimiter)
        {
            reader.SetDelimiter(specifier.delimiter);
            continue;
        }

        if (specifier.isLiteral)
        {
            if (!reader.Literal(specifier.literal))
            {
                result.error = "Expected \"" + specifier.literal + "\"";
                result.specifier = number + 1;
                result.values.clear();
                return result;
            }

            continue;
        }

        ++number;

        if (result.values.size() >= MaximumValues)
        {
            throw std::length_error("Format produces more than 64 values");
        }

        auto field = reader.Field(TakesRest(specifiers, index));

        try
        {
            if (!field || field->empty())
            {
                if (!specifier.optional)
                {
                    throw Failure{std::string("Missing ") + Describe(specifier.type)};
                }

                result.values.push_back(specifier.fallback ? Convert(specifier, *specifier.fallback, resolver, true) : Value::Nil());
                continue;
            }

            result.values.push_back(Convert(specifier, *field, resolver, false));
        }
        catch (const Failure& failure)
        {
            result.error = failure.message;
            result.specifier = number;
            result.values.clear();
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace Scanner
