#include "SDK/Plugin.h"
#include "Scanner.h"

#include <algorithm>
#include <cctype>
#include <deque>

using namespace SDK;

class CSscanf final : public CPlugin
{
    bool OnLoad() override
    {
        RawFunction("sscanf", &CSscanf::InvokeScan, this);

        Function("sscanfTable", [this](std::optional<std::string> input, std::string format)
        {
            const auto result = Run(input.value_or(""), format);

            if (!result.success)
            {
                return std::make_tuple(CValue{}, std::optional<std::string>(result.error), std::optional<int64_t>(static_cast<int64_t>(result.specifier)));
            }

            CTable table;

            for (size_t index = 0; index < result.values.size(); ++index)
            {
                table.Set(static_cast<int64_t>(index + 1), ToValue(result.values[index]));
            }

            table.Set("n", static_cast<int64_t>(result.values.size()));

            return std::make_tuple(CValue(table), std::optional<std::string>{}, std::optional<int64_t>{});
        });

        return true;
    }

    static int InvokeScan(const Value* arguments, size_t count, Value* output, size_t capacity, void* data)
    {
        auto& self = *static_cast<CSscanf*>(data);

        if (count != 2)
        {
            throw std::invalid_argument("sscanf expects (input, format)");
        }

        std::string input;

        if (arguments[0].type == PLUGIN_VALUE_STRING)
        {
            input.assign(arguments[0].stringValue ? arguments[0].stringValue : "", arguments[0].stringLength);
        }
        else if (arguments[0].type != PLUGIN_VALUE_NIL)
        {
            throw std::invalid_argument("Expected a string or nil at argument 1");
        }

        if (arguments[1].type != PLUGIN_VALUE_STRING || !arguments[1].stringValue)
        {
            throw std::invalid_argument("Expected a format string at argument 2");
        }

        const std::string format(arguments[1].stringValue, arguments[1].stringLength);
        const auto result = self.Run(input, format);

        self.m_strings.clear();

        if (!result.success)
        {
            if (capacity < 3)
            {
                throw std::length_error("Not enough result slots");
            }

            self.m_strings.push_back(result.error);
            output[0] = ValueBoolean(false);
            output[1] = ValueStringSized(self.m_strings.back().data(), self.m_strings.back().size());
            output[2] = ValueInteger(static_cast<int64_t>(result.specifier));

            return 3;
        }

        if (result.values.size() + 1 > capacity)
        {
            throw std::length_error("sscanf can return at most " + std::to_string(capacity - 1) + " values; use sscanfTable for more");
        }

        output[0] = ValueBoolean(true);

        for (size_t index = 0; index < result.values.size(); ++index)
        {
            const auto& value = result.values[index];

            switch (value.kind)
            {
            case Scanner::Kind::Integer:
                output[index + 1] = ValueInteger(value.integer);
                break;
            case Scanner::Kind::Number:
                output[index + 1] = ValueNumber(value.number);
                break;
            case Scanner::Kind::Boolean:
                output[index + 1] = ValueBoolean(value.boolean);
                break;
            case Scanner::Kind::String:
                self.m_strings.push_back(value.text);
                output[index + 1] = ValueStringSized(self.m_strings.back().data(), self.m_strings.back().size());
                break;
            default:
                output[index + 1] = ValueNil();
                break;
            }
        }

        return static_cast<int>(result.values.size() + 1);
    }

    Scanner::Result Run(const std::string& input, const std::string& format)
    {
        return Scanner::Scan(input, format, [this](const std::string& query, Scanner::PlayerFilter filter)
        {
            return FindPlayer(query, filter);
        });
    }

    static CValue ToValue(const Scanner::Value& value)
    {
        switch (value.kind)
        {
        case Scanner::Kind::Integer:
            return CValue(value.integer);
        case Scanner::Kind::Number:
            return CValue(value.number);
        case Scanner::Kind::Boolean:
            return CValue(value.boolean);
        case Scanner::Kind::String:
            return CValue(value.text);
        default:
            return CValue{};
        }
    }

    static std::string Normalize(std::string text)
    {
        for (auto& character : text)
        {
            character = character == '_' ? ' ' : static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        return text;
    }

    std::optional<int64_t> FindPlayer(const std::string& query, Scanner::PlayerFilter filter)
    {
        const auto* api = API();

        if (!api || !api->player || query.empty())
        {
            return std::nullopt;
        }

        const bool humans = filter != Scanner::PlayerFilter::Bot;
        const bool bots = filter != Scanner::PlayerFilter::Human;

        bool numeric = query.size() <= 9;

        for (const char character : query)
        {
            numeric = numeric && std::isdigit(static_cast<unsigned char>(character)) != 0;
        }

        if (numeric)
        {
            const int id = std::stoi(query);
            const bool connected = api->player->IsConnected && api->player->IsConnected(id);
            const bool npc = connected && api->player->IsNPC && api->player->IsNPC(id);
            const bool human = humans && (api->GetEntityGeneration(PLUGIN_ENTITY_PLAYER, id) != 0 || (connected && !npc));
            const bool bot = bots && (api->GetEntityGeneration(PLUGIN_ENTITY_NPC, id) != 0 || npc);

            if (human || bot)
            {
                return id;
            }

            return std::nullopt;
        }

        const auto needle = Normalize(query);
        std::vector<int> exact;
        std::vector<int> prefix;
        std::vector<int> partial;

        std::vector<int> seen;

        const auto consider = [&](int id)
        {
            if (std::find(seen.begin(), seen.end(), id) != seen.end())
            {
                return;
            }

            seen.push_back(id);
            const char* raw = api->player->GetName(id);

            if (!raw)
            {
                return;
            }

            const auto name = Normalize(raw);

            if (name == needle)
            {
                exact.push_back(id);
            }
            else if (name.compare(0, needle.size(), needle) == 0)
            {
                prefix.push_back(id);
            }
            else if (name.find(needle) != std::string::npos)
            {
                partial.push_back(id);
            }
        };

        const auto fromEntity = [&](const CEntity& entity)
        {
            consider(entity.Id());
        };

        if (humans)
        {
            Players.ForEach(fromEntity);
        }

        if (bots)
        {
            NPCs.ForEach(fromEntity);
        }

        for (const auto* matches : {&exact, &prefix, &partial})
        {
            if (matches->size() == 1)
            {
                return matches->front();
            }

            if (matches->size() > 1)
            {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::deque<std::string> m_strings;
};

REGISTER_PLUGIN(CSscanf)
