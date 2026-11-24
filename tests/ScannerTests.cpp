#include "Scanner.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
int g_failures = 0;
int g_checks = 0;

void Check(bool condition, const char* expression, int line)
{
    ++g_checks;

    if (!condition)
    {
        ++g_failures;
        std::printf("FAILED line %d: %s\n", line, expression);
    }
}

#define CHECK(condition) Check((condition), #condition, __LINE__)

std::optional<int64_t> Resolve(const std::string& query, Scanner::PlayerFilter filter)
{
    if (query == "0" || query == "Jims_Smp" || query == "jims")
    {
        return filter == Scanner::PlayerFilter::Bot ? std::nullopt : std::optional<int64_t>(0);
    }

    if (query == "7" || query == "Fare_1")
    {
        return filter == Scanner::PlayerFilter::Human ? std::nullopt : std::optional<int64_t>(7);
    }

    return std::nullopt;
}

Scanner::Result Scan(const char* input, const char* format)
{
    return Scanner::Scan(input, format, Resolve);
}

bool Throws(const char* input, const char* format)
{
    try
    {
        Scan(input, format);
    }
    catch (const std::exception&)
    {
        return true;
    }

    return false;
}

} // namespace

int main()
{
    {
        const auto result = Scan("5 250", "ud");
        CHECK(result.success);
        CHECK(result.values[0].kind == Scanner::Kind::Nil);
        CHECK(result.values[1].integer == 250);
    }

    {
        const auto result = Scan("0 250", "ud");
        CHECK(result.success);
        CHECK(result.values.size() == 2);
        CHECK(result.values[0].integer == 0);
        CHECK(result.values[1].integer == 250);
    }

    {
        const auto result = Scan("jims 99", "ud");
        CHECK(result.success);
        CHECK(result.values[0].kind == Scanner::Kind::Integer && result.values[0].integer == 0);
    }

    {
        const auto result = Scan("nobody 99", "ud");
        CHECK(result.success);
        CHECK(result.values[0].kind == Scanner::Kind::Nil);
        CHECK(result.values[1].integer == 99);
    }

    {
        const auto result = Scan("Fare_1", "r");
        CHECK(result.success);
        CHECK(result.values[0].kind == Scanner::Kind::Nil);
        CHECK(Scan("Fare_1", "q").values[0].integer == 7);
    }

    {
        const auto result = Scan("0 hello there friend", "us");
        CHECK(result.success);
        CHECK(result.values[1].text == "hello there friend");
    }

    {
        const auto result = Scan("hello there 5", "wd");
        CHECK(!result.success);
        CHECK(result.specifier == 2);
    }

    {
        const auto result = Scan("hello 5", "s[3]d");
        CHECK(result.success);
        CHECK(result.values[0].text == "hel");
        CHECK(result.values[1].integer == 5);
    }

    {
        const auto result = Scan("12.5 -3 0x1F 1012 777 ff", "fiibox");
        CHECK(!result.success);
        CHECK(result.specifier == 4);
        CHECK(result.error.find("binary") != std::string::npos);
    }

    {
        const auto result = Scan("12.5 -3 0x1F 1010 777 ff", "fiiboh");
        CHECK(result.success);
        CHECK(std::fabs(result.values[0].number - 12.5) < 1e-9);
        CHECK(result.values[1].integer == -3);
        CHECK(result.values[2].integer == 31);
        CHECK(result.values[3].integer == 10);
        CHECK(result.values[4].integer == 511);
        CHECK(result.values[5].integer == 255);
    }

    {
        const auto result = Scan("12abc", "d");
        CHECK(!result.success);
        CHECK(result.error.find("whole number") != std::string::npos);
    }

    {
        const auto result = Scan("", "d");
        CHECK(!result.success);
        CHECK(result.error == "Missing a whole number");
    }

    {
        const auto result = Scan("", "I(5)S(none)F(1.5)D");
        CHECK(result.success);
        CHECK(result.values[0].integer == 5);
        CHECK(result.values[1].text == "none");
        CHECK(std::fabs(result.values[2].number - 1.5) < 1e-9);
        CHECK(result.values[3].kind == Scanner::Kind::Nil);
    }

    {
        const auto result = Scan("7", "dZ");
        CHECK(result.success);
        CHECK(result.values[1].kind == Scanner::Kind::Nil);
    }

    {
        const auto result = Scan("yes off 1", "lll");
        CHECK(result.success);
        CHECK(result.values[0].boolean && !result.values[1].boolean && result.values[2].boolean);
    }

    {
        const auto result = Scan("a", "c");
        CHECK(result.success && result.values[0].text == "a");
        CHECK(!Scan("ab", "c").success);
    }

    {
        const auto result = Scan("12, Kero Konbini ,  4.5", "p<,>dsf");
        CHECK(result.success);
        CHECK(result.values[1].text == "Kero Konbini");
        CHECK(std::fabs(result.values[2].number - 4.5) < 1e-9);
    }

    {
        const auto result = Scan("1,,3", "p<,>dDd");
        CHECK(result.success);
        CHECK(result.values[1].kind == Scanner::Kind::Nil);
        CHECK(result.values[2].integer == 3);
    }

    {
        const auto result = Scan("give 0 money 500", "'give' u 'money' d");
        CHECK(result.success);
        CHECK(result.values.size() == 2);
        CHECK(result.values[1].integer == 500);
        CHECK(!Scan("take 0 money 500", "'give' u 'money' d").success);
    }

    {
        const auto result = Scan("9223372036854775807 -9223372036854775808", "dd");
        CHECK(result.success);
        CHECK(!Scan("9223372036854775808", "d").success);
    }

    {
        const auto result = Scan("U", "U(-1)");
        CHECK(result.success);
        CHECK(Scan("", "U(-1)").values[0].kind == Scanner::Kind::Nil);
        CHECK(Scan("", "U(3)").values[0].integer == 3);
    }

    CHECK(Throws("1", "y"));
    CHECK(Throws("1", "d(5)"));
    CHECK(Throws("1", "'oops"));
    CHECK(Throws("1", "s[0]"));
    CHECK(Throws("1", "p,"));

    std::printf("%d checks, %d failed\n", g_checks, g_failures);

    return g_failures == 0 ? 0 : 1;
}
