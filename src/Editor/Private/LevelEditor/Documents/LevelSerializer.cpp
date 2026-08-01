#include "Editor/LevelEditor/Documents/LevelSerializer.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        /*
         * Version 6:
         * CharacterAnimation хранит только
         * ссылку на внешний JSON-профиль.
         */
        constexpr std::uint64_t
            CurrentFormatVersion = 6U;

        /*
         * Version 5:
         * CharacterAnimationSet был встроен
         * непосредственно в level.
         */
        constexpr std::uint64_t
            InlineCharacterAnimationFormatVersion =
                5U;

        constexpr std::uint64_t
            ModularCharacterFormatVersion = 4U;

        constexpr std::uint64_t
            SingleMeshCharacterFormatVersion = 3U;

        constexpr std::uint64_t
            ComponentFormatVersion = 2U;

        constexpr std::uint64_t
            LegacyFormatVersion = 1U;

        constexpr std::string_view FormatName =
            "LTS.Level";

        constexpr std::string_view
            CharacterAnimationProfileFormatName =
                "LTS.CharacterAnimationProfile";

        constexpr std::uint64_t
            CharacterAnimationProfileFormatVersion =
                1U;

        constexpr std::uintmax_t MaximumLevelFileSize =
            64U * 1024U * 1024U;

        struct JsonValue final
        {
            enum class Type : std::uint8_t
            {
                Null = 0,
                Boolean,
                Number,
                String,
                Array,
                Object
            };

            Type type = Type::Null;

            bool boolean = false;
            double number = 0.0;

            std::string string;

            std::vector<JsonValue> array;

            std::vector<
                std::pair<std::string, JsonValue>>
                    object;

            [[nodiscard]]
            const JsonValue* Find(
                const std::string_view key) const noexcept
            {
                for (const auto& entry : object)
                {
                    if (entry.first == key)
                    {
                        return &entry.second;
                    }
                }

                return nullptr;
            }
        };

        class JsonParser final
        {
        public:
            explicit JsonParser(
                const std::string_view text) noexcept
                : text_(text)
            {
            }

            [[nodiscard]]
            bool Parse(
                JsonValue& value,
                std::string& error)
            {
                error.clear();
                position_ = 0U;

                if (!ParseValue(value, error))
                {
                    return false;
                }

                SkipWhitespace();

                if (position_ != text_.size())
                {
                    error =
                        "Unexpected data after JSON document.";

                    return false;
                }

                return true;
            }

        private:
            void SkipWhitespace() noexcept
            {
                while (position_ < text_.size())
                {
                    const char character =
                        text_[position_];

                    if (
                        character != ' ' &&
                        character != '\t' &&
                        character != '\r' &&
                        character != '\n')
                    {
                        break;
                    }

                    ++position_;
                }
            }

            [[nodiscard]]
            bool ParseValue(
                JsonValue& value,
                std::string& error)
            {
                SkipWhitespace();

                if (position_ >= text_.size())
                {
                    error =
                        "Unexpected end of JSON document.";

                    return false;
                }

                switch (text_[position_])
                {
                    case '{':
                        return ParseObject(
                            value,
                            error);

                    case '[':
                        return ParseArray(
                            value,
                            error);

                    case '"':
                        value.type =
                            JsonValue::Type::String;

                        return ParseString(
                            value.string,
                            error);

                    case 't':
                        return ParseLiteral(
                            "true",
                            JsonValue::Type::Boolean,
                            value,
                            true,
                            error);

                    case 'f':
                        return ParseLiteral(
                            "false",
                            JsonValue::Type::Boolean,
                            value,
                            false,
                            error);

                    case 'n':
                        return ParseLiteral(
                            "null",
                            JsonValue::Type::Null,
                            value,
                            false,
                            error);

                    default:
                        return ParseNumber(
                            value,
                            error);
                }
            }

            [[nodiscard]]
            bool ParseObject(
                JsonValue& value,
                std::string& error)
            {
                value = {};
                value.type =
                    JsonValue::Type::Object;

                ++position_;
                SkipWhitespace();

                if (
                    position_ < text_.size() &&
                    text_[position_] == '}')
                {
                    ++position_;
                    return true;
                }

                while (position_ < text_.size())
                {
                    std::string key;

                    if (!ParseString(key, error))
                    {
                        return false;
                    }

                    SkipWhitespace();

                    if (
                        position_ >= text_.size() ||
                        text_[position_] != ':')
                    {
                        error =
                            "Expected ':' after object key.";

                        return false;
                    }

                    ++position_;

                    JsonValue child;

                    if (!ParseValue(child, error))
                    {
                        return false;
                    }

                    value.object.emplace_back(
                        std::move(key),
                        std::move(child));

                    SkipWhitespace();

                    if (position_ >= text_.size())
                    {
                        error =
                            "Unexpected end of JSON object.";

                        return false;
                    }

                    if (text_[position_] == '}')
                    {
                        ++position_;
                        return true;
                    }

                    if (text_[position_] != ',')
                    {
                        error =
                            "Expected ',' inside JSON object.";

                        return false;
                    }

                    ++position_;
                    SkipWhitespace();
                }

                error =
                    "Unexpected end of JSON object.";

                return false;
            }

            [[nodiscard]]
            bool ParseArray(
                JsonValue& value,
                std::string& error)
            {
                value = {};
                value.type =
                    JsonValue::Type::Array;

                ++position_;
                SkipWhitespace();

                if (
                    position_ < text_.size() &&
                    text_[position_] == ']')
                {
                    ++position_;
                    return true;
                }

                while (position_ < text_.size())
                {
                    JsonValue child;

                    if (!ParseValue(child, error))
                    {
                        return false;
                    }

                    value.array.push_back(
                        std::move(child));

                    SkipWhitespace();

                    if (position_ >= text_.size())
                    {
                        error =
                            "Unexpected end of JSON array.";

                        return false;
                    }

                    if (text_[position_] == ']')
                    {
                        ++position_;
                        return true;
                    }

                    if (text_[position_] != ',')
                    {
                        error =
                            "Expected ',' inside JSON array.";

                        return false;
                    }

                    ++position_;
                }

                error =
                    "Unexpected end of JSON array.";

                return false;
            }

            [[nodiscard]]
            bool ParseString(
                std::string& output,
                std::string& error)
            {
                SkipWhitespace();

                if (
                    position_ >= text_.size() ||
                    text_[position_] != '"')
                {
                    error =
                        "Expected a JSON string.";

                    return false;
                }

                ++position_;
                output.clear();

                while (position_ < text_.size())
                {
                    const char character =
                        text_[position_++];

                    if (character == '"')
                    {
                        return true;
                    }

                    if (
                        static_cast<unsigned char>(
                            character) < 0x20U)
                    {
                        error =
                            "Invalid control character in JSON string.";

                        return false;
                    }

                    if (character != '\\')
                    {
                        output.push_back(character);
                        continue;
                    }

                    if (position_ >= text_.size())
                    {
                        error =
                            "Invalid JSON string escape.";

                        return false;
                    }

                    switch (text_[position_++])
                    {
                        case '"':
                            output.push_back('"');
                            break;

                        case '\\':
                            output.push_back('\\');
                            break;

                        case '/':
                            output.push_back('/');
                            break;

                        case 'b':
                            output.push_back('\b');
                            break;

                        case 'f':
                            output.push_back('\f');
                            break;

                        case 'n':
                            output.push_back('\n');
                            break;

                        case 'r':
                            output.push_back('\r');
                            break;

                        case 't':
                            output.push_back('\t');
                            break;

                        default:
                            error =
                                "Unsupported JSON string escape.";

                            return false;
                    }
                }

                error =
                    "Unexpected end of JSON string.";

                return false;
            }

            [[nodiscard]]
            bool ParseNumber(
                JsonValue& value,
                std::string& error)
            {
                const std::size_t start =
                    position_;

                if (
                    position_ < text_.size() &&
                    text_[position_] == '-')
                {
                    ++position_;
                }

                while (
                    position_ < text_.size() &&
                    text_[position_] >= '0' &&
                    text_[position_] <= '9')
                {
                    ++position_;
                }

                if (
                    position_ < text_.size() &&
                    text_[position_] == '.')
                {
                    ++position_;

                    while (
                        position_ < text_.size() &&
                        text_[position_] >= '0' &&
                        text_[position_] <= '9')
                    {
                        ++position_;
                    }
                }

                if (
                    position_ < text_.size() &&
                    (
                        text_[position_] == 'e' ||
                        text_[position_] == 'E'
                    ))
                {
                    ++position_;

                    if (
                        position_ < text_.size() &&
                        (
                            text_[position_] == '+' ||
                            text_[position_] == '-'
                        ))
                    {
                        ++position_;
                    }

                    while (
                        position_ < text_.size() &&
                        text_[position_] >= '0' &&
                        text_[position_] <= '9')
                    {
                        ++position_;
                    }
                }

                if (start == position_)
                {
                    error =
                        "Expected a JSON value.";

                    return false;
                }

                const std::string token(
                    text_.substr(
                        start,
                        position_ - start));

                char* end = nullptr;

                const double number =
                    std::strtod(
                        token.c_str(),
                        &end);

                if (
                    end == nullptr ||
                    end == token.c_str() ||
                    *end != '\0' ||
                    !std::isfinite(number))
                {
                    error =
                        "Invalid JSON number.";

                    return false;
                }

                value = {};
                value.type =
                    JsonValue::Type::Number;

                value.number = number;

                return true;
            }

            [[nodiscard]]
            bool ParseLiteral(
                const std::string_view literal,
                const JsonValue::Type type,
                JsonValue& value,
                const bool booleanValue,
                std::string& error)
            {
                if (
                    text_.substr(
                        position_,
                        literal.size()) != literal)
                {
                    error =
                        "Invalid JSON literal.";

                    return false;
                }

                position_ += literal.size();

                value = {};
                value.type = type;
                value.boolean = booleanValue;

                return true;
            }

            std::string_view text_;
            std::size_t position_ = 0U;
        };

        [[nodiscard]]
        bool ToUtf8(
            const std::wstring_view input,
            std::string& output) noexcept
        {
            output.clear();

            if (input.empty())
            {
                return true;
            }

            const int required =
                WideCharToMultiByte(
                    CP_UTF8,
                    WC_ERR_INVALID_CHARS,
                    input.data(),
                    static_cast<int>(
                        input.size()),
                    nullptr,
                    0,
                    nullptr,
                    nullptr);

            if (required <= 0)
            {
                return false;
            }

            output.resize(
                static_cast<std::size_t>(
                    required));

            return WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                input.data(),
                static_cast<int>(
                    input.size()),
                output.data(),
                required,
                nullptr,
                nullptr) == required;
        }

        [[nodiscard]]
        bool FromUtf8(
            const std::string_view input,
            std::wstring& output) noexcept
        {
            output.clear();

            if (input.empty())
            {
                return true;
            }

            const int required =
                MultiByteToWideChar(
                    CP_UTF8,
                    MB_ERR_INVALID_CHARS,
                    input.data(),
                    static_cast<int>(
                        input.size()),
                    nullptr,
                    0);

            if (required <= 0)
            {
                return false;
            }

            output.resize(
                static_cast<std::size_t>(
                    required));

            return MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                input.data(),
                static_cast<int>(
                    input.size()),
                output.data(),
                required) == required;
        }

        void WriteJsonString(
            std::ostream& output,
            const std::string_view value)
        {
            output.put('"');

            for (
                const unsigned char character :
                value)
            {
                switch (character)
                {
                    case '"':
                        output << "\\\"";
                        break;

                    case '\\':
                        output << "\\\\";
                        break;

                    case '\b':
                        output << "\\b";
                        break;

                    case '\f':
                        output << "\\f";
                        break;

                    case '\n':
                        output << "\\n";
                        break;

                    case '\r':
                        output << "\\r";
                        break;

                    case '\t':
                        output << "\\t";
                        break;

                    default:
                        if (character >= 0x20U)
                        {
                            output.put(
                                static_cast<char>(
                                    character));
                        }
                        break;
                }
            }

            output.put('"');
        }

        void WriteVector3(
            std::ostream& output,
            const std::array<float, 3U>& value)
        {
            output
                << '['
                << value[0]
                << ", "
                << value[1]
                << ", "
                << value[2]
                << ']';
        }

        [[nodiscard]]
        const char* KindToString(
            const EditorEntityKind kind) noexcept
        {
            switch (kind)
            {
                case EditorEntityKind::Environment:
                    return "Environment";

                case EditorEntityKind::DirectionalLight:
                    return "DirectionalLight";

                case EditorEntityKind::SpawnPoint:
                    return "SpawnPoint";

                case EditorEntityKind::Anomaly:
                    return "Anomaly";

                case EditorEntityKind::LootContainer:
                    return "LootContainer";

                case EditorEntityKind::Terrain:
                    return "Terrain";
                
                case EditorEntityKind::Character:
                    return "Character";

                case EditorEntityKind::Empty:
                default:
                    return "Empty";
            }
        }

        [[nodiscard]]
        bool StringToKind(
            const std::string_view value,
            EditorEntityKind& kind) noexcept
        {
            if (value == "Environment")
            {
                kind = EditorEntityKind::Environment;
            }
            else if (value == "DirectionalLight")
            {
                kind =
                    EditorEntityKind::DirectionalLight;
            }
            else if (value == "SpawnPoint")
            {
                kind = EditorEntityKind::SpawnPoint;
            }
            else if (value == "Anomaly")
            {
                kind = EditorEntityKind::Anomaly;
            }
            else if (value == "LootContainer")
            {
                kind =
                    EditorEntityKind::LootContainer;
            }
            else if (value == "Terrain")
            {
                kind = EditorEntityKind::Terrain;
            }
            else if (value == "Character")
            {
                kind = EditorEntityKind::Character;
            }
            else if (value == "Empty")
            {
                kind = EditorEntityKind::Empty;
            }
            else
            {
                return false;
            }

            return true;
        }

        [[nodiscard]]
        const char* SkyPresetToString(const engine::scene::SkyPreset preset) noexcept
        {
            switch (preset)
            {
            case engine::scene::SkyPreset::ClearDay:
                return "ClearDay";

            case engine::scene::SkyPreset::Cloudy:
                return "Cloudy";

            case engine::scene::SkyPreset::Sunrise:
                return "Sunrise";

            case engine::scene::SkyPreset::Sunset:
                return "Sunset";

            case engine::scene::SkyPreset::Night:
                return "Night";

            case engine::scene::SkyPreset::Storm:
                return "Storm";

            case engine::scene::SkyPreset::Custom:
                return "Custom";
            }

            return "Custom";
        }

        [[nodiscard]]
        bool StringToSkyPreset(const std::string_view value, engine::scene::SkyPreset& preset) noexcept
        {
            if (value == "ClearDay")
                preset = engine::scene::SkyPreset::ClearDay;
            else if (value == "Cloudy")
                preset = engine::scene::SkyPreset::Cloudy;
            else if (value == "Sunrise")
                preset = engine::scene::SkyPreset::Sunrise;
            else if (value == "Sunset")
                preset = engine::scene::SkyPreset::Sunset;
            else if (value == "Night")
                preset = engine::scene::SkyPreset::Night;
            else if (value == "Storm")
                preset = engine::scene::SkyPreset::Storm;
            else if (value == "Custom")
                preset = engine::scene::SkyPreset::Custom;
            else
                return false;

            return true;
        }

        [[nodiscard]]
        const JsonValue* RequireField(const JsonValue& object, const std::string_view name) noexcept
        {
            if (
                object.type !=
                JsonValue::Type::Object)
            {
                return nullptr;
            }

            return object.Find(name);
        }

        [[nodiscard]]
        bool ReadString(
            const JsonValue* value,
            std::string& output) noexcept
        {
            if (
                value == nullptr ||
                value->type !=
                    JsonValue::Type::String)
            {
                return false;
            }

            output = value->string;
            return true;
        }

        [[nodiscard]]
        bool ReadBoolean(
            const JsonValue* value,
            bool& output) noexcept
        {
            if (
                value == nullptr ||
                value->type !=
                    JsonValue::Type::Boolean)
            {
                return false;
            }

            output = value->boolean;
            return true;
        }

        [[nodiscard]]
        bool ReadFloat(
            const JsonValue* value,
            float& output) noexcept
        {
            if (
                value == nullptr ||
                value->type !=
                    JsonValue::Type::Number ||
                !std::isfinite(value->number) ||
                value->number <
                    -static_cast<double>(
                        std::numeric_limits<float>::max()) ||
                value->number >
                    static_cast<double>(
                        std::numeric_limits<float>::max()))
            {
                return false;
            }

            output =
                static_cast<float>(
                    value->number);

            return true;
        }

        [[nodiscard]]
        bool ReadUnsigned(
            const JsonValue* value,
            std::uint64_t& output) noexcept
        {
            if (
                value == nullptr ||
                value->type !=
                    JsonValue::Type::Number ||
                !std::isfinite(value->number) ||
                value->number < 0.0 ||
                std::floor(value->number) !=
                    value->number ||
                value->number >
                    static_cast<double>(
                        std::numeric_limits<
                            std::uint64_t>::max()))
            {
                return false;
            }

            output =
                static_cast<std::uint64_t>(
                    value->number);

            return true;
        }

        [[nodiscard]]
        bool ReadVector3(
            const JsonValue* value,
            std::array<float, 3U>& output) noexcept
        {
            if (
                value == nullptr ||
                value->type !=
                    JsonValue::Type::Array ||
                value->array.size() != 3U)
            {
                return false;
            }

            for (
                std::size_t index = 0U;
                index < output.size();
                ++index)
            {
                if (!ReadFloat(
                        &value->array[index],
                        output[index]))
                {
                    return false;
                }
            }

            return true;
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteWideString(
            std::ostream& output,
            const std::wstring& value)
        {
            std::string utf8;

            if (!ToUtf8(value, utf8))
            {
                return false;
            }

            WriteJsonString(output, utf8);
            return output.good();
        }

        [[nodiscard]]
        bool ReadWideStringField(
            const JsonValue& object,
            const std::string_view name,
            std::wstring& output)
        {
            std::string utf8;

            return
                ReadString(
                    RequireField(
                        object,
                        name),
                    utf8) &&
                FromUtf8(
                    utf8,
                    output);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteDirectionalAnimationSet(
            std::ostream& output,
            const engine::scene::
                CharacterDirectionalAnimationSet& value)
        {
            output << "{\"forward\": ";

            if (!WriteWideString(
                    output,
                    value.forward))
            {
                return false;
            }

            output << ", \"forwardLeft\": ";

            if (!WriteWideString(
                    output,
                    value.forwardLeft))
            {
                return false;
            }

            output << ", \"forwardRight\": ";

            if (!WriteWideString(
                    output,
                    value.forwardRight))
            {
                return false;
            }

            output << ", \"left\": ";

            if (!WriteWideString(
                    output,
                    value.left))
            {
                return false;
            }

            output << ", \"right\": ";

            if (!WriteWideString(
                    output,
                    value.right))
            {
                return false;
            }

            output << ", \"backward\": ";

            if (!WriteWideString(
                    output,
                    value.backward))
            {
                return false;
            }

            output << ", \"backwardLeft\": ";

            if (!WriteWideString(
                    output,
                    value.backwardLeft))
            {
                return false;
            }

            output << ", \"backwardRight\": ";

            if (!WriteWideString(
                    output,
                    value.backwardRight))
            {
                return false;
            }

            output << '}';
            return output.good();
        }

        [[nodiscard]]
        bool ReadDirectionalAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterDirectionalAnimationSet& value)
        {
            return
                ReadWideStringField(
                    json,
                    "forward",
                    value.forward) &&
                ReadWideStringField(
                    json,
                    "forwardLeft",
                    value.forwardLeft) &&
                ReadWideStringField(
                    json,
                    "forwardRight",
                    value.forwardRight) &&
                ReadWideStringField(
                    json,
                    "left",
                    value.left) &&
                ReadWideStringField(
                    json,
                    "right",
                    value.right) &&
                ReadWideStringField(
                    json,
                    "backward",
                    value.backward) &&
                ReadWideStringField(
                    json,
                    "backwardLeft",
                    value.backwardLeft) &&
                ReadWideStringField(
                    json,
                    "backwardRight",
                    value.backwardRight);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteSprintAnimationSet(
            std::ostream& output,
            const engine::scene::
                CharacterSprintAnimationSet& value)
        {
            output << "{\"forward\": ";

            if (!WriteWideString(
                    output,
                    value.forward))
            {
                return false;
            }

            output << ", \"forwardLeft\": ";

            if (!WriteWideString(
                    output,
                    value.forwardLeft))
            {
                return false;
            }

            output << ", \"forwardRight\": ";

            if (!WriteWideString(
                    output,
                    value.forwardRight))
            {
                return false;
            }

            output << '}';
            return output.good();
        }

        [[nodiscard]]
        bool ReadSprintAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterSprintAnimationSet& value)
        {
            return
                ReadWideStringField(
                    json,
                    "forward",
                    value.forward) &&
                ReadWideStringField(
                    json,
                    "forwardLeft",
                    value.forwardLeft) &&
                ReadWideStringField(
                    json,
                    "forwardRight",
                    value.forwardRight);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteUpperBodyAnimationSet(
            std::ostream& output,
            const engine::scene::
                CharacterUpperBodyAnimationSet& value)
        {
            output << "{\"standingRelaxedIdle\": ";

            if (!WriteWideString(
                    output,
                    value.standingRelaxedIdle))
            {
                return false;
            }

            output << ", \"standingRelaxedMove\": ";

            if (!WriteWideString(
                    output,
                    value.standingRelaxedMove))
            {
                return false;
            }

            output << ", \"standingAimIdle\": ";

            if (!WriteWideString(
                    output,
                    value.standingAimIdle))
            {
                return false;
            }

            output << ", \"standingAimMove\": ";

            if (!WriteWideString(
                    output,
                    value.standingAimMove))
            {
                return false;
            }

            output << ", \"crouchedRelaxedIdle\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedRelaxedIdle))
            {
                return false;
            }

            output << ", \"crouchedRelaxedMove\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedRelaxedMove))
            {
                return false;
            }

            output << ", \"crouchedAimIdle\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedAimIdle))
            {
                return false;
            }

            output << ", \"crouchedAimMove\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedAimMove))
            {
                return false;
            }

            output << '}';
            return output.good();
        }

        [[nodiscard]]
        bool ReadUpperBodyAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterUpperBodyAnimationSet& value)
        {
            return
                ReadWideStringField(
                    json,
                    "standingRelaxedIdle",
                    value.standingRelaxedIdle) &&
                ReadWideStringField(
                    json,
                    "standingRelaxedMove",
                    value.standingRelaxedMove) &&
                ReadWideStringField(
                    json,
                    "standingAimIdle",
                    value.standingAimIdle) &&
                ReadWideStringField(
                    json,
                    "standingAimMove",
                    value.standingAimMove) &&
                ReadWideStringField(
                    json,
                    "crouchedRelaxedIdle",
                    value.crouchedRelaxedIdle) &&
                ReadWideStringField(
                    json,
                    "crouchedRelaxedMove",
                    value.crouchedRelaxedMove) &&
                ReadWideStringField(
                    json,
                    "crouchedAimIdle",
                    value.crouchedAimIdle) &&
                ReadWideStringField(
                    json,
                    "crouchedAimMove",
                    value.crouchedAimMove);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteActionAnimationSet(
            std::ostream& output,
            const engine::scene::
                CharacterActionAnimationSet& value)
        {
            output << "{\"primaryStanding\": ";

            if (!WriteWideString(
                    output,
                    value.primaryStanding))
            {
                return false;
            }

            output << ", \"primaryMoving\": ";

            if (!WriteWideString(
                    output,
                    value.primaryMoving))
            {
                return false;
            }

            output << ", \"primaryCrouched\": ";

            if (!WriteWideString(
                    output,
                    value.primaryCrouched))
            {
                return false;
            }

            output << ", \"secondaryStanding\": ";

            if (!WriteWideString(
                    output,
                    value.secondaryStanding))
            {
                return false;
            }

            output << ", \"secondaryMoving\": ";

            if (!WriteWideString(
                    output,
                    value.secondaryMoving))
            {
                return false;
            }

            output << ", \"secondaryCrouched\": ";

            if (!WriteWideString(
                    output,
                    value.secondaryCrouched))
            {
                return false;
            }

            output << ", \"reloadStanding\": ";

            if (!WriteWideString(
                    output,
                    value.reloadStanding))
            {
                return false;
            }

            output << ", \"reloadMoving\": ";

            if (!WriteWideString(
                    output,
                    value.reloadMoving))
            {
                return false;
            }

            output << ", \"reloadCrouched\": ";

            if (!WriteWideString(
                    output,
                    value.reloadCrouched))
            {
                return false;
            }

            output << '}';
            return output.good();
        }

        [[nodiscard]]
        bool ReadActionAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterActionAnimationSet& value)
        {
            return
                ReadWideStringField(
                    json,
                    "primaryStanding",
                    value.primaryStanding) &&
                ReadWideStringField(
                    json,
                    "primaryMoving",
                    value.primaryMoving) &&
                ReadWideStringField(
                    json,
                    "primaryCrouched",
                    value.primaryCrouched) &&
                ReadWideStringField(
                    json,
                    "secondaryStanding",
                    value.secondaryStanding) &&
                ReadWideStringField(
                    json,
                    "secondaryMoving",
                    value.secondaryMoving) &&
                ReadWideStringField(
                    json,
                    "secondaryCrouched",
                    value.secondaryCrouched) &&
                ReadWideStringField(
                    json,
                    "reloadStanding",
                    value.reloadStanding) &&
                ReadWideStringField(
                    json,
                    "reloadMoving",
                    value.reloadMoving) &&
                ReadWideStringField(
                    json,
                    "reloadCrouched",
                    value.reloadCrouched);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteViewAnimationSet(
            std::ostream& output,
            const engine::scene::
                CharacterViewAnimationSet& value)
        {
            output << "{\"upperBody\": ";

            if (!WriteUpperBodyAnimationSet(
                    output,
                    value.upperBody))
            {
                return false;
            }

            output << ", \"actions\": ";

            if (!WriteActionAnimationSet(
                    output,
                    value.actions))
            {
                return false;
            }

            output << '}';
            return output.good();
        }

        [[nodiscard]]
        bool ReadViewAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterViewAnimationSet& value)
        {
            const JsonValue* const upperBody =
                RequireField(
                    json,
                    "upperBody");

            const JsonValue* const actions =
                RequireField(
                    json,
                    "actions");

            return
                upperBody != nullptr &&
                actions != nullptr &&
                ReadUpperBodyAnimationSet(
                    *upperBody,
                    value.upperBody) &&
                ReadActionAnimationSet(
                    *actions,
                    value.actions);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteLowerBodyAnimationSet(
            std::ostream& output,
            const engine::scene::
                CharacterLowerBodyAnimationSet& value)
        {
            output << "{\"standingIdle\": ";

            if (!WriteWideString(
                    output,
                    value.standingIdle))
            {
                return false;
            }

            output << ", \"walk\": ";

            if (!WriteDirectionalAnimationSet(
                    output,
                    value.walk))
            {
                return false;
            }

            output << ", \"run\": ";

            if (!WriteDirectionalAnimationSet(
                    output,
                    value.run))
            {
                return false;
            }

            output << ", \"sprint\": ";

            if (!WriteSprintAnimationSet(
                    output,
                    value.sprint))
            {
                return false;
            }

            output << ", \"crouchedIdle\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedIdle))
            {
                return false;
            }

            output << ", \"crouchedMove\": ";

            if (!WriteDirectionalAnimationSet(
                    output,
                    value.crouchedMove))
            {
                return false;
            }

            output << ", \"turnInPlaceLeft\": ";

            if (!WriteWideString(
                    output,
                    value.turnInPlaceLeft))
            {
                return false;
            }

            output << ", \"turnInPlaceRight\": ";

            if (!WriteWideString(
                    output,
                    value.turnInPlaceRight))
            {
                return false;
            }

            output
                << ", \"crouchedTurnInPlaceLeft\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedTurnInPlaceLeft))
            {
                return false;
            }

            output
                << ", \"crouchedTurnInPlaceRight\": ";

            if (!WriteWideString(
                    output,
                    value.crouchedTurnInPlaceRight))
            {
                return false;
            }

            output << ", \"jumpStart\": ";

            if (!WriteWideString(
                    output,
                    value.jumpStart))
            {
                return false;
            }

            output << ", \"jumpLoop\": ";

            if (!WriteWideString(
                    output,
                    value.jumpLoop))
            {
                return false;
            }

            output << ", \"jumpLand\": ";

            if (!WriteWideString(
                    output,
                    value.jumpLand))
            {
                return false;
            }

            output << '}';
            return output.good();
        }

        [[nodiscard]]
        bool ReadLowerBodyAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterLowerBodyAnimationSet& value)
        {
            const JsonValue* const walk =
                RequireField(
                    json,
                    "walk");

            const JsonValue* const run =
                RequireField(
                    json,
                    "run");

            const JsonValue* const sprint =
                RequireField(
                    json,
                    "sprint");

            const JsonValue* const crouchedMove =
                RequireField(
                    json,
                    "crouchedMove");

            return
                walk != nullptr &&
                run != nullptr &&
                sprint != nullptr &&
                crouchedMove != nullptr &&
                ReadWideStringField(
                    json,
                    "standingIdle",
                    value.standingIdle) &&
                ReadDirectionalAnimationSet(
                    *walk,
                    value.walk) &&
                ReadDirectionalAnimationSet(
                    *run,
                    value.run) &&
                ReadSprintAnimationSet(
                    *sprint,
                    value.sprint) &&
                ReadWideStringField(
                    json,
                    "crouchedIdle",
                    value.crouchedIdle) &&
                ReadDirectionalAnimationSet(
                    *crouchedMove,
                    value.crouchedMove) &&
                ReadWideStringField(
                    json,
                    "turnInPlaceLeft",
                    value.turnInPlaceLeft) &&
                ReadWideStringField(
                    json,
                    "turnInPlaceRight",
                    value.turnInPlaceRight) &&
                ReadWideStringField(
                    json,
                    "crouchedTurnInPlaceLeft",
                    value.crouchedTurnInPlaceLeft) &&
                ReadWideStringField(
                    json,
                    "crouchedTurnInPlaceRight",
                    value.crouchedTurnInPlaceRight) &&
                ReadWideStringField(
                    json,
                    "jumpStart",
                    value.jumpStart) &&
                ReadWideStringField(
                    json,
                    "jumpLoop",
                    value.jumpLoop) &&
                ReadWideStringField(
                    json,
                    "jumpLand",
                    value.jumpLand);
        }

        [[maybe_unused]]
        [[nodiscard]]
        bool WriteAnimationTuning(
            std::ostream& output,
            const engine::scene::
                CharacterAnimationTuning& value)
        {
            output
                << "{\"locomotionBlendSeconds\": "
                << value.locomotionBlendSeconds
                << ", \"upperBodyBlendSeconds\": "
                << value.upperBodyBlendSeconds
                << ", \"actionBlendInSeconds\": "
                << value.actionBlendInSeconds
                << ", \"actionBlendOutSeconds\": "
                << value.actionBlendOutSeconds
                << ", \"maximumUpperBodyYawDegrees\": "
                << value.maximumUpperBodyYawDegrees
                << ", \"turnInPlaceEnterDegrees\": "
                << value.turnInPlaceEnterDegrees
                << ", \"turnInPlaceExitDegrees\": "
                << value.turnInPlaceExitDegrees
                << ", \"turnInPlaceSpeedDegrees\": "
                << value.turnInPlaceSpeedDegrees
                << '}';

            return output.good();
        }

        [[nodiscard]]
        bool ReadAnimationTuning(
            const JsonValue& json,
            engine::scene::
                CharacterAnimationTuning& value)
        {
            return
                ReadFloat(
                    RequireField(
                        json,
                        "locomotionBlendSeconds"),
                    value.locomotionBlendSeconds) &&
                ReadFloat(
                    RequireField(
                        json,
                        "upperBodyBlendSeconds"),
                    value.upperBodyBlendSeconds) &&
                ReadFloat(
                    RequireField(
                        json,
                        "actionBlendInSeconds"),
                    value.actionBlendInSeconds) &&
                ReadFloat(
                    RequireField(
                        json,
                        "actionBlendOutSeconds"),
                    value.actionBlendOutSeconds) &&
                ReadFloat(
                    RequireField(
                        json,
                        "maximumUpperBodyYawDegrees"),
                    value.maximumUpperBodyYawDegrees) &&
                ReadFloat(
                    RequireField(
                        json,
                        "turnInPlaceEnterDegrees"),
                    value.turnInPlaceEnterDegrees) &&
                ReadFloat(
                    RequireField(
                        json,
                        "turnInPlaceExitDegrees"),
                    value.turnInPlaceExitDegrees) &&
                ReadFloat(
                    RequireField(
                        json,
                        "turnInPlaceSpeedDegrees"),
                    value.turnInPlaceSpeedDegrees);
        }

        [[nodiscard]]
        bool WriteCharacterAnimationComponent(
            std::ostream& output,
            const engine::scene::
                CharacterAnimationComponent& value)
        {
            std::string profilePath;

            if (!ToUtf8(
                    value.profilePath,
                    profilePath))
            {
                return false;
            }

            output
                << "{\"enabled\": "
                << (
                    value.enabled
                        ? "true"
                        : "false"
                )
                << ", \"profile\": ";

            WriteJsonString(
                output,
                profilePath);

            output << '}';

            return output.good();
        }

        [[nodiscard]]
        bool ReadCharacterAnimationSet(
            const JsonValue& json,
            engine::scene::
                CharacterAnimationSet& value)
        {
            const JsonValue* const lowerBody =
                RequireField(
                    json,
                    "lowerBody");

            const JsonValue* const thirdPerson =
                RequireField(
                    json,
                    "thirdPerson");

            const JsonValue* const firstPerson =
                RequireField(
                    json,
                    "firstPerson");

            const JsonValue* const tuning =
                RequireField(
                    json,
                    "tuning");

            return
                lowerBody != nullptr &&
                thirdPerson != nullptr &&
                firstPerson != nullptr &&
                tuning != nullptr &&

                ReadString(
                    RequireField(
                        json,
                        "upperBodyRootBone"),
                    value.upperBodyRootBone) &&

                ReadString(
                    RequireField(
                        json,
                        "actionRootBone"),
                    value.actionRootBone) &&

                ReadString(
                    RequireField(
                        json,
                        "lookRootBone"),
                    value.lookRootBone) &&

                ReadLowerBodyAnimationSet(
                    *lowerBody,
                    value.lowerBody) &&

                ReadViewAnimationSet(
                    *thirdPerson,
                    value.thirdPerson) &&

                ReadViewAnimationSet(
                    *firstPerson,
                    value.firstPerson) &&

                ReadAnimationTuning(
                    *tuning,
                    value.tuning);
        }

        [[nodiscard]]
        bool ReadCharacterAnimationComponent(
            const JsonValue& json,
            engine::scene::
                CharacterAnimationComponent& value)
        {
            if (
                !ReadBoolean(
                    RequireField(
                        json,
                        "enabled"),
                    value.enabled))
            {
                return false;
            }

            /*
             * Level version 6:
             *
             * {
             *     "enabled": true,
             *     "profile": "Data/Config/..."
             * }
             */
            if (
                const JsonValue* const profileField =
                    json.Find("profile"))
            {
                std::string profilePath;

                if (
                    !ReadString(
                        profileField,
                        profilePath) ||
                    !FromUtf8(
                        profilePath,
                        value.profilePath))
                {
                    return false;
                }

                value.animationSet = {};
                value.runtime.Reset();

                value.profileLoaded = false;
                value.profileError.clear();

                return true;
            }

            /*
             * Совместимость с version 5,
             * где полный Animation Set хранился
             * непосредственно внутри level.
             */
            if (!ReadCharacterAnimationSet(
                    json,
                    value.animationSet))
            {
                return false;
            }

            value.profilePath =
                L"Data/Config/CharacterAnimations/"
                L"MEL_Hands.json";

            value.runtime.Reset();

            /*
             * Оставляем загруженный inline-набор
             * как fallback, если внешний профиль
             * пока отсутствует.
             */
            value.profileLoaded = false;
            value.profileError.clear();

            return true;
        }

        [[nodiscard]]
        bool ParseLegacyEntity(
            const JsonValue& jsonEntity,
            EditorSceneEntity& entity)
        {
            std::uint64_t id = 0U;

            std::string kind;
            std::string name;

            if (
                !ReadUnsigned(
                    RequireField(
                        jsonEntity,
                        "id"),
                    id) ||
                !ReadString(
                    RequireField(
                        jsonEntity,
                        "kind"),
                    kind) ||
                !ReadString(
                    RequireField(
                        jsonEntity,
                        "name"),
                    name) ||
                !ReadVector3(
                    RequireField(
                        jsonEntity,
                        "position"),
                    entity.transform.position) ||
                !ReadVector3(
                    RequireField(
                        jsonEntity,
                        "rotation"),
                    entity.transform.
                        rotationDegrees) ||
                !ReadVector3(
                    RequireField(
                        jsonEntity,
                        "scale"),
                    entity.transform.scale) ||
                !StringToKind(
                    kind,
                    entity.kind) ||
                !FromUtf8(
                    name,
                    entity.name))
            {
                return false;
            }

            entity.id =
                static_cast<EditorEntityId>(id);

            engine::scene::SceneWorld::
                EnsureDefaultComponents(entity);

            return true;
        }

        [[nodiscard]]
        bool ParseComponentEntity(
            const JsonValue& jsonEntity,
            EditorSceneEntity& entity)
        {
            std::uint64_t id = 0U;
            std::string kind;

            if (
                !ReadUnsigned(
                    RequireField(
                        jsonEntity,
                        "id"),
                    id) ||
                !ReadString(
                    RequireField(
                        jsonEntity,
                        "kind"),
                    kind) ||
                !StringToKind(
                    kind,
                    entity.kind))
            {
                return false;
            }

            entity.id =
                static_cast<EditorEntityId>(id);

            const JsonValue* const components =
                RequireField(
                    jsonEntity,
                    "components");

            if (
                components == nullptr ||
                components->type !=
                    JsonValue::Type::Object)
            {
                return false;
            }

            const JsonValue* const nameComponent =
                RequireField(
                    *components,
                    "Name");

            const JsonValue* const transformComponent =
                RequireField(
                    *components,
                    "Transform");

            std::string name;

            if (
                nameComponent == nullptr ||
                transformComponent == nullptr ||
                !ReadString(
                    RequireField(
                        *nameComponent,
                        "value"),
                    name) ||
                !FromUtf8(
                    name,
                    entity.name) ||
                !ReadVector3(
                    RequireField(
                        *transformComponent,
                        "position"),
                    entity.transform.position) ||
                !ReadVector3(
                    RequireField(
                        *transformComponent,
                        "rotation"),
                    entity.transform.
                        rotationDegrees) ||
                !ReadVector3(
                    RequireField(
                        *transformComponent,
                        "scale"),
                    entity.transform.scale))
            {
                return false;
            }

            if (const JsonValue* component = RequireField(*components, "Environment"))
            {
                engine::scene::EnvironmentComponent value;
                std::string asset;

                if (!ReadString(
                        RequireField(*component, "asset"),
                        asset) ||
                    !FromUtf8(asset, value.environmentAsset))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("preset"))
                {
                    std::string preset;

                    if (!ReadString(field, preset) ||
                        !StringToSkyPreset(preset, value.preset))
                    {
                        return false;
                    }
                }

                if (const JsonValue* field = component->Find("topColor");
                    field != nullptr && !ReadVector3(field, value.topColor))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("horizonColor");
                    field != nullptr && !ReadVector3(field, value.horizonColor))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("groundColor");
                    field != nullptr && !ReadVector3(field, value.groundColor))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("ambientColor");
                    field != nullptr && !ReadVector3(field, value.ambientColor))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("skyIntensity");
                    field != nullptr && !ReadFloat(field, value.skyIntensity))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("ambientIntensity");
                    field != nullptr && !ReadFloat(field, value.ambientIntensity))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("horizonExponent");
                    field != nullptr && !ReadFloat(field, value.horizonExponent))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("sunDiskSizeDegrees");
                    field != nullptr &&
                    !ReadFloat(field, value.sunDiskSizeDegrees))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("visible");
                    field != nullptr && !ReadBoolean(field, value.visible))
                {
                    return false;
                }

                if (const JsonValue* field = component->Find("linkSun");
                    field != nullptr && !ReadBoolean(field, value.linkSun))
                {
                    return false;
                }

                entity.environment = std::move(value);
            }

            if (const JsonValue* hierarchy = components->Find("EditorHierarchy"))
            {
                std::uint64_t parentId = 0U;
                std::string folder;
                if (!ReadUnsigned(hierarchy->Find("parentId"), parentId) ||
                    !ReadString(hierarchy->Find("folder"), folder) ||
                    !FromUtf8(folder, entity.editorFolder))
                {
                    return false;
                }
                entity.parentId = static_cast<EditorEntityId>(parentId);
            }

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "StaticMesh"))
            {
                engine::scene::StaticMeshComponent
                    value;

                std::string asset;

                if (
                    !ReadString(
                        RequireField(
                            *component,
                            "asset"),
                        asset) ||
                    !FromUtf8(
                        asset,
                        value.assetPath) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "visible"),
                        value.visible) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "castShadows"),
                        value.castShadows))
                {
                    return false;
                }

                entity.staticMesh =
                    std::move(value);
            }

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "SkeletalMesh"))
            {
                engine::scene::SkeletalMeshComponent
                    value;

                std::string family;
                std::string skeleton;
                std::string idleAnimation;
                std::string walkAnimation;
                std::string runAnimation;
                std::string jumpAnimation;

                /*
                 * В level версии 3 поля family ещё не было.
                 */
                if (
                    const JsonValue* familyField =
                        component->Find("family"))
                {
                    if (
                        !ReadString(
                            familyField,
                            family) ||
                        !FromUtf8(
                            family,
                            value.characterFamily))
                    {
                        return false;
                    }
                }

                if (
                    !ReadString(
                        RequireField(
                            *component,
                            "skeleton"),
                        skeleton) ||
                    !ReadString(
                        RequireField(
                            *component,
                            "idleAnimation"),
                        idleAnimation) ||
                    !ReadString(
                        RequireField(
                            *component,
                            "walkAnimation"),
                        walkAnimation) ||
                    !ReadString(
                        RequireField(
                            *component,
                            "runAnimation"),
                        runAnimation) ||
                    !ReadString(
                        RequireField(
                            *component,
                            "jumpAnimation"),
                        jumpAnimation) ||
                    !FromUtf8(
                        skeleton,
                        value.skeletonPath) ||
                    !FromUtf8(
                        idleAnimation,
                        value.idleAnimation) ||
                    !FromUtf8(
                        walkAnimation,
                        value.walkAnimation) ||
                    !FromUtf8(
                        runAnimation,
                        value.runAnimation) ||
                    !FromUtf8(
                        jumpAnimation,
                        value.jumpAnimation) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "visible"),
                        value.visible) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "castShadows"),
                        value.castShadows))
                {
                    return false;
                }

                if (
                    const JsonValue* autoFpsField =
                        component->Find(
                            "autoFirstPersonBody"))
                {
                    if (!ReadBoolean(
                            autoFpsField,
                            value.autoFirstPersonBody))
                    {
                        return false;
                    }
                }

                const auto readPart =
                    [&value](
                        const JsonValue& parts,
                        const char* const name,
                        const engine::scene::
                            CharacterMeshSlot slot)
                    {
                        const JsonValue* const partJson =
                            parts.Find(name);

                        /*
                         * Отсутствующий слот разрешён.
                         *
                         * Например Hair может быть пустым.
                         */
                        if (partJson == nullptr)
                        {
                            return true;
                        }

                        std::string asset;

                        auto& part =
                            value.GetPart(slot);

                        /*
                         * Поддерживаем сокращённую запись:
                         *
                         * "hair": "Data/.../hair_01.skm"
                         */
                        if (
                            partJson->type ==
                            JsonValue::Type::String)
                        {
                            if (!ReadString(
                                    partJson,
                                    asset))
                            {
                                return false;
                            }
                        }
                        /*
                         * Основной формат:
                         *
                         * "hair": {
                         *     "asset": "...",
                         *     "visible": true
                         * }
                         */
                        else if (
                            partJson->type ==
                            JsonValue::Type::Object)
                        {
                            if (!ReadString(
                                    RequireField(
                                        *partJson,
                                        "asset"),
                                    asset))
                            {
                                return false;
                            }

                            if (
                                const JsonValue*
                                    visibleField =
                                        partJson->Find(
                                            "visible"))
                            {
                                if (!ReadBoolean(
                                        visibleField,
                                        part.visible))
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            return false;
                        }

                        return FromUtf8(
                            asset,
                            part.assetPath);
                    };

                if (
                    const JsonValue* parts =
                        component->Find("parts"))
                {
                    if (
                        parts->type !=
                        JsonValue::Type::Object ||
                        !readPart(
                            *parts,
                            "hair",
                            engine::scene::
                                CharacterMeshSlot::Hair) ||
                        !readPart(
                            *parts,
                            "head",
                            engine::scene::
                                CharacterMeshSlot::Head) ||
                        !readPart(
                            *parts,
                            "body",
                            engine::scene::
                                CharacterMeshSlot::Body) ||
                        !readPart(
                            *parts,
                            "legs",
                            engine::scene::
                                CharacterMeshSlot::Legs) ||
                        !readPart(
                            *parts,
                            "shoes",
                            engine::scene::
                                CharacterMeshSlot::Shoes) ||
                        !readPart(
                            *parts,
                            "firstPersonBody",
                            engine::scene::
                                CharacterMeshSlot::
                                    FirstPersonBody))
                    {
                        return false;
                    }
                }
                else
                {
                    /*
                     * Совместимость с level версии 3.
                     *
                     * Старый одиночный asset переносим в Body.
                     */
                    const JsonValue* const legacyAsset =
                        component->Find("asset");

                    if (legacyAsset != nullptr)
                    {
                        std::string asset;

                        if (
                            !ReadString(
                                legacyAsset,
                                asset) ||
                            !FromUtf8(
                                asset,
                                value.GetPart(
                                    engine::scene::
                                        CharacterMeshSlot::
                                            Body)
                                    .assetPath))
                        {
                            return false;
                        }
                    }
                }

                entity.skeletalMesh =
                    std::move(value);
            }

            /*
             * CharacterAnimation отсутствует
             * в level версиях 1-4.
             *
             * В этом случае SceneWorld создаст
             * компонент с настройками по умолчанию.
             */
            if (
                const JsonValue* component =
                    components->Find(
                        "CharacterAnimation"))
            {
                engine::scene::
                    CharacterAnimationComponent value;

                if (
                    !ReadCharacterAnimationComponent(
                        *component,
                        value))
                {
                    return false;
                }

                entity.characterAnimation =
                    std::move(value);
            }

            if (const JsonValue* component = RequireField(*components, "CharacterController"))
            {
                engine::scene::CharacterControllerComponent value;

                if (
                    !ReadFloat(
                        RequireField(
                            *component,
                            "capsuleRadius"),
                        value.capsuleRadius) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "capsuleHeight"),
                        value.capsuleHeight) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "walkSpeed"),
                        value.walkSpeed) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "runSpeed"),
                        value.runSpeed) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "acceleration"),
                        value.acceleration) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "deceleration"),
                        value.deceleration) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "rotationSpeedDegrees"),
                        value.rotationSpeedDegrees) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "jumpVelocity"),
                        value.jumpVelocity) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "gravity"),
                        value.gravity) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "playerControlled"),
                        value.playerControlled) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "useRootMotion"),
                        value.useRootMotion))
                {
                    return false;
                }

                entity.characterController = value;
            }

            if (const JsonValue* component = RequireField(*components, "Terrain"))
            {
                engine::scene::TerrainComponent value;
                std::string asset;
                if (!ReadString(RequireField(*component, "asset"), asset) ||
                    !FromUtf8(asset, value.assetPath) ||
                    !ReadBoolean(RequireField(*component, "visible"), value.visible) ||
                    !ReadBoolean(RequireField(*component, "castShadows"), value.castShadows))
                {
                    return false;
                }
                if (const JsonValue* layers = component->Find("layers"))
                {
                    if (layers->type != JsonValue::Type::Array) return false;
                    for (const JsonValue& layerJson : layers->array)
                    {
                        engine::scene::TerrainComponent::LayerOverride layer;
                        if (layerJson.type != JsonValue::Type::Object ||
                            !ReadString(RequireField(layerJson, "name"), layer.name) ||
                            !ReadString(RequireField(layerJson, "diffuse"), layer.diffusePath) ||
                            !ReadString(RequireField(layerJson, "normal"), layer.normalPath) ||
                            !ReadFloat(RequireField(layerJson, "scaleU"), layer.scaleU) ||
                            !ReadFloat(RequireField(layerJson, "scaleV"), layer.scaleV) ||
                            !ReadBoolean(RequireField(layerJson, "visible"), layer.visible)) return false;
                        if (const JsonValue* offset = layerJson.Find("offsetU");
                            offset != nullptr && !ReadFloat(offset, layer.offsetU)) return false;
                        if (const JsonValue* offset = layerJson.Find("offsetV");
                            offset != nullptr && !ReadFloat(offset, layer.offsetV)) return false;
                        value.layers.push_back(std::move(layer));
                    }
                }
                entity.terrain = std::move(value);
            }

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "DirectionalLight"))
            {
                engine::scene::
                    DirectionalLightComponent value;

                if (
                    !ReadVector3(
                        RequireField(
                            *component,
                            "color"),
                        value.color) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "intensity"),
                        value.intensity) ||
                    !ReadBoolean(
                        RequireField(
                            *component,
                            "castShadows"),
                        value.castShadows))
                {
                    return false;
                }

                entity.directionalLight =
                    value;
            }

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "SpawnPoint"))
            {
                engine::scene::SpawnPointComponent
                    value;

                std::string tag;

                if (
                    !ReadString(
                        RequireField(
                            *component,
                            "tag"),
                        tag) ||
                    !FromUtf8(
                        tag,
                        value.spawnTag))
                {
                    return false;
                }

                entity.spawnPoint =
                    std::move(value);
            }

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "Anomaly"))
            {
                engine::scene::AnomalyComponent value;

                std::string type;

                if (
                    !ReadString(
                        RequireField(
                            *component,
                            "type"),
                        type) ||
                    !FromUtf8(
                        type,
                        value.anomalyType) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "radius"),
                        value.radius) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "damagePerSecond"),
                        value.damagePerSecond))
                {
                    return false;
                }

                entity.anomaly =
                    std::move(value);
            }

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "LootContainer"))
            {
                engine::scene::
                    LootContainerComponent value;

                std::string lootTable;

                if (
                    !ReadString(
                        RequireField(
                            *component,
                            "lootTable"),
                        lootTable) ||
                    !FromUtf8(
                        lootTable,
                        value.lootTable) ||
                    !ReadFloat(
                        RequireField(
                            *component,
                            "respawnSeconds"),
                        value.respawnSeconds))
                {
                    return false;
                }

                entity.lootContainer =
                    std::move(value);
            }

            engine::scene::SceneWorld::
                EnsureDefaultComponents(entity);

            return true;
        }

        [[nodiscard]]
        bool Validate(
            const EditorLevelFileData& data,
            std::wstring& error)
        {
            if (
                data.name.empty() ||
                data.guid.empty())
            {
                error =
                    L"Level metadata is incomplete.";

                return false;
            }

            std::unordered_set<EditorEntityId> ids;

            EditorEntityId maximumId = 0U;

            for (
                const EditorSceneEntity& entity :
                data.snapshot.entities)
            {
                if (
                    entity.id == 0U ||
                    entity.name.empty() ||
                    !ids.insert(entity.id).second ||
                    !engine::scene::SceneWorld::
                        IsFiniteTransform(
                            entity.transform))
                {
                    error =
                        L"The level contains an invalid entity.";

                    return false;
                }

                maximumId =
                    std::max(
                        maximumId,
                        entity.id);
            }

            if (
                data.snapshot.nextEntityId == 0U ||
                data.snapshot.nextEntityId <=
                    maximumId)
            {
                error =
                    L"nextEntityId is invalid.";

                return false;
            }

            if (
                data.snapshot.selectedIndex !=
                    InvalidEditorEntityIndex &&
                data.snapshot.selectedIndex >=
                    data.snapshot.entities.size())
            {
                error =
                    L"Selected entity index is invalid.";

                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool BuildJson(
            const EditorLevelFileData& data,
            std::string& outputText,
            std::wstring& error)
        {
            std::string levelName;
            std::string levelGuid;

            if (
                !ToUtf8(data.name, levelName) ||
                !ToUtf8(data.guid, levelGuid))
            {
                error =
                    L"Failed to convert level metadata to UTF-8.";

                return false;
            }

            std::ostringstream output;

            output.imbue(
                std::locale::classic());

            output << std::setprecision(9);

            output << "{\n";
            output << "  \"format\": ";

            WriteJsonString(output, FormatName);

            output
                << ",\n  \"version\": "
                << CurrentFormatVersion
                << ",\n  \"name\": ";

            WriteJsonString(output, levelName);

            output << ",\n  \"guid\": ";

            WriteJsonString(output, levelGuid);

            output
                << ",\n  \"nextEntityId\": "
                << data.snapshot.nextEntityId
                << ",\n  \"selectedIndex\": ";

            if (
                data.snapshot.selectedIndex ==
                InvalidEditorEntityIndex)
            {
                output << "null";
            }
            else
            {
                output
                    << data.snapshot.selectedIndex;
            }

            output << ",\n  \"entities\": [\n";

            for (
                std::size_t index = 0U;
                index <
                    data.snapshot.entities.size();
                ++index)
            {
                const EditorSceneEntity& entity =
                    data.snapshot.entities[index];

                std::string entityName;
                std::string editorFolder;

                if (!ToUtf8(
                        entity.name,
                        entityName) ||
                    !ToUtf8(entity.editorFolder, editorFolder))
                {
                    error =
                        L"Failed to convert an entity name to UTF-8.";

                    return false;
                }

                output
                    << "    {\n"
                    << "      \"id\": "
                    << entity.id
                    << ",\n"
                    << "      \"kind\": ";

                WriteJsonString(
                    output,
                    KindToString(entity.kind));

                output
                    << ",\n"
                    << "      \"components\": {\n"
                    << "        \"Name\": {\"value\": ";

                WriteJsonString(
                    output,
                    entityName);

                output
                    << "},\n"
                    << "        \"Transform\": {"
                    << "\"position\": ";

                WriteVector3(
                    output,
                    entity.transform.position);

                output << ", \"rotation\": ";

                WriteVector3(
                    output,
                    entity.transform.rotationDegrees);

                output << ", \"scale\": ";

                WriteVector3(
                    output,
                    entity.transform.scale);

                output << "},\n        \"EditorHierarchy\": {\"parentId\": "
                       << entity.parentId << ", \"folder\": ";
                WriteJsonString(output, editorFolder);
                output << '}';

                if (entity.environment.has_value())
                {
                    const auto& environment = *entity.environment;

                    std::string asset;

                    if (!ToUtf8(environment.environmentAsset, asset))
                    {
                        error =
                            L"Failed to convert an environment asset path.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"Environment\": {\"asset\": ";

                    WriteJsonString(output, asset);

                    output << ", \"preset\": ";

                    WriteJsonString(
                        output,
                        SkyPresetToString(environment.preset));

                    output << ", \"topColor\": ";
                    WriteVector3(output, environment.topColor);

                    output << ", \"horizonColor\": ";
                    WriteVector3(output, environment.horizonColor);

                    output << ", \"groundColor\": ";
                    WriteVector3(output, environment.groundColor);

                    output << ", \"ambientColor\": ";
                    WriteVector3(output, environment.ambientColor);

                    output
                        << ", \"skyIntensity\": "
                        << environment.skyIntensity
                        << ", \"ambientIntensity\": "
                        << environment.ambientIntensity
                        << ", \"horizonExponent\": "
                        << environment.horizonExponent
                        << ", \"sunDiskSizeDegrees\": "
                        << environment.sunDiskSizeDegrees
                        << ", \"visible\": "
                        << (environment.visible ? "true" : "false")
                        << ", \"linkSun\": "
                        << (environment.linkSun ? "true" : "false")
                        << '}';
                }

                if (entity.staticMesh.has_value())
                {
                    std::string asset;

                    if (!ToUtf8(
                            entity.staticMesh->assetPath,
                            asset))
                    {
                        error =
                            L"Failed to convert a mesh asset path.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"StaticMesh\": {\"asset\": ";

                    WriteJsonString(output, asset);

                    output
                        << ", \"visible\": "
                        << (
                            entity.staticMesh->visible
                                ? "true"
                                : "false"
                        )
                        << ", \"castShadows\": "
                        << (
                            entity.staticMesh->castShadows
                                ? "true"
                                : "false"
                        )
                        << '}';
                }

                if (entity.skeletalMesh.has_value())
                {
                    const engine::scene::
                        SkeletalMeshComponent&
                            skeletalMesh =
                                *entity.skeletalMesh;

                    std::string family;
                    std::string skeleton;
                    std::string idleAnimation;
                    std::string walkAnimation;
                    std::string runAnimation;
                    std::string jumpAnimation;

                    if (
                        !ToUtf8(
                            skeletalMesh.characterFamily,
                            family) ||
                        !ToUtf8(
                            skeletalMesh.skeletonPath,
                            skeleton) ||
                        !ToUtf8(
                            skeletalMesh.idleAnimation,
                            idleAnimation) ||
                        !ToUtf8(
                            skeletalMesh.walkAnimation,
                            walkAnimation) ||
                        !ToUtf8(
                            skeletalMesh.runAnimation,
                            runAnimation) ||
                        !ToUtf8(
                            skeletalMesh.jumpAnimation,
                            jumpAnimation))
                    {
                        error =
                            L"Failed to convert modular "
                            L"character paths to UTF-8.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"SkeletalMesh\": {"
                        << "\"family\": ";

                    WriteJsonString(
                        output,
                        family);

                    output << ", \"skeleton\": ";

                    WriteJsonString(
                        output,
                        skeleton);

                    output
                        << ", \"parts\": {";

                    const auto writePart =
                        [&output,
                         &skeletalMesh](
                            const char* const name,
                            const engine::scene::
                                CharacterMeshSlot slot,
                            const bool first)
                        {
                            const auto& part =
                                skeletalMesh.GetPart(
                                    slot);

                            std::string asset;

                            if (!ToUtf8(
                                    part.assetPath,
                                    asset))
                            {
                                return false;
                            }

                            if (!first)
                            {
                                output << ',';
                            }

                            output
                                << "\n"
                                << "          \""
                                << name
                                << "\": {\"asset\": ";

                            WriteJsonString(
                                output,
                                asset);

                            output
                                << ", \"visible\": "
                                << (
                                    part.visible
                                        ? "true"
                                        : "false"
                                )
                                << '}';

                            return true;
                        };

                    if (
                        !writePart(
                            "hair",
                            engine::scene::
                                CharacterMeshSlot::Hair,
                            true) ||
                        !writePart(
                            "head",
                            engine::scene::
                                CharacterMeshSlot::Head,
                            false) ||
                        !writePart(
                            "body",
                            engine::scene::
                                CharacterMeshSlot::Body,
                            false) ||
                        !writePart(
                            "legs",
                            engine::scene::
                                CharacterMeshSlot::Legs,
                            false) ||
                        !writePart(
                            "shoes",
                            engine::scene::
                                CharacterMeshSlot::Shoes,
                            false) ||
                        !writePart(
                            "firstPersonBody",
                            engine::scene::
                                CharacterMeshSlot::
                                    FirstPersonBody,
                            false))
                    {
                        error =
                            L"Failed to convert a modular "
                            L"character part path.";

                        return false;
                    }

                    output
                        << "\n"
                        << "        }"
                        << ", \"idleAnimation\": ";

                    WriteJsonString(
                        output,
                        idleAnimation);

                    output
                        << ", \"walkAnimation\": ";

                    WriteJsonString(
                        output,
                        walkAnimation);

                    output
                        << ", \"runAnimation\": ";

                    WriteJsonString(
                        output,
                        runAnimation);

                    output
                        << ", \"jumpAnimation\": ";

                    WriteJsonString(
                        output,
                        jumpAnimation);

                    output
                        << ", \"visible\": "
                        << (
                            skeletalMesh.visible
                                ? "true"
                                : "false"
                        )
                        << ", \"castShadows\": "
                        << (
                            skeletalMesh.castShadows
                                ? "true"
                                : "false"
                        )
                        << ", \"autoFirstPersonBody\": "
                        << (
                            skeletalMesh.
                                autoFirstPersonBody
                                    ? "true"
                                    : "false"
                        )
                        << '}';
                }

                if (
                    entity.characterAnimation.
                        has_value())
                {
                    output
                        << ",\n"
                        << "        "
                        << "\"CharacterAnimation\": ";

                    if (
                        !WriteCharacterAnimationComponent(
                            output,
                            *entity.characterAnimation))
                    {
                        error =
                            L"Failed to serialize the "
                            L"character animation set.";

                        return false;
                    }
                }

                if (entity.characterController.has_value())
                {
                    const engine::scene::CharacterControllerComponent&
                        controller =
                            *entity.characterController;

                    output
                        << ",\n"
                        << "        \"CharacterController\": {"
                        << "\"capsuleRadius\": "
                        << controller.capsuleRadius
                        << ", \"capsuleHeight\": "
                        << controller.capsuleHeight
                        << ", \"walkSpeed\": "
                        << controller.walkSpeed
                        << ", \"runSpeed\": "
                        << controller.runSpeed
                        << ", \"acceleration\": "
                        << controller.acceleration
                        << ", \"deceleration\": "
                        << controller.deceleration
                        << ", \"rotationSpeedDegrees\": "
                        << controller.rotationSpeedDegrees
                        << ", \"jumpVelocity\": "
                        << controller.jumpVelocity
                        << ", \"gravity\": "
                        << controller.gravity
                        << ", \"playerControlled\": "
                        << (
                            controller.playerControlled
                                ? "true"
                                : "false"
                        )
                        << ", \"useRootMotion\": "
                        << (
                            controller.useRootMotion
                                ? "true"
                                : "false"
                        )
                        << '}';
                }

                if (entity.terrain.has_value())
                {
                    std::string asset;
                    if (!ToUtf8(entity.terrain->assetPath, asset))
                    {
                        error = L"Failed to convert a terrain asset path.";
                        return false;
                    }
                    output << ",\n        \"Terrain\": {\"asset\": ";
                    WriteJsonString(output, asset);
                    output << ", \"visible\": "
                           << (entity.terrain->visible ? "true" : "false")
                           << ", \"castShadows\": "
                           << (entity.terrain->castShadows ? "true" : "false")
                           << ", \"layers\": [";
                    for (std::size_t layerIndex = 0U;
                         layerIndex < entity.terrain->layers.size(); ++layerIndex)
                    {
                        if (layerIndex != 0U) output << ", ";
                        const auto& layer = entity.terrain->layers[layerIndex];
                        output << "{\"name\": "; WriteJsonString(output, layer.name);
                        output << ", \"diffuse\": "; WriteJsonString(output, layer.diffusePath);
                        output << ", \"normal\": "; WriteJsonString(output, layer.normalPath);
                        output << ", \"scaleU\": " << layer.scaleU
                               << ", \"scaleV\": " << layer.scaleV
                               << ", \"offsetU\": " << layer.offsetU
                               << ", \"offsetV\": " << layer.offsetV
                               << ", \"visible\": " << (layer.visible ? "true" : "false") << '}';
                    }
                    output << "]}";
                }

                if (entity.directionalLight.has_value())
                {
                    output
                        << ",\n"
                        << "        \"DirectionalLight\": {\"color\": ";

                    WriteVector3(
                        output,
                        entity.directionalLight->color);

                    output
                        << ", \"intensity\": "
                        << entity.directionalLight->
                            intensity
                        << ", \"castShadows\": "
                        << (
                            entity.directionalLight->
                                castShadows
                                ? "true"
                                : "false"
                        )
                        << '}';
                }

                if (entity.spawnPoint.has_value())
                {
                    std::string tag;

                    if (!ToUtf8(
                            entity.spawnPoint->spawnTag,
                            tag))
                    {
                        error =
                            L"Failed to convert a spawn tag.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"SpawnPoint\": {\"tag\": ";

                    WriteJsonString(output, tag);

                    output << '}';
                }

                if (entity.anomaly.has_value())
                {
                    std::string type;

                    if (!ToUtf8(
                            entity.anomaly->anomalyType,
                            type))
                    {
                        error =
                            L"Failed to convert an anomaly type.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"Anomaly\": {\"type\": ";

                    WriteJsonString(output, type);

                    output
                        << ", \"radius\": "
                        << entity.anomaly->radius
                        << ", \"damagePerSecond\": "
                        << entity.anomaly->
                            damagePerSecond
                        << '}';
                }

                if (entity.lootContainer.has_value())
                {
                    std::string lootTable;

                    if (!ToUtf8(
                            entity.lootContainer->
                                lootTable,
                            lootTable))
                    {
                        error =
                            L"Failed to convert a loot table.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"LootContainer\": {\"lootTable\": ";

                    WriteJsonString(
                        output,
                        lootTable);

                    output
                        << ", \"respawnSeconds\": "
                        << entity.lootContainer->
                            respawnSeconds
                        << '}';
                }

                output
                    << "\n"
                    << "      }\n"
                    << "    }";

                if (
                    index + 1U <
                    data.snapshot.entities.size())
                {
                    output << ',';
                }

                output << '\n';
            }

            output
                << "  ]\n"
                << "}\n";

            if (!output.good())
            {
                error =
                    L"Failed to build level JSON.";

                return false;
            }

            outputText = output.str();
            return true;
        }

        [[nodiscard]]
        bool ParseJson(
            const std::string& contents,
            EditorLevelFileData& data,
            std::wstring& error)
        {
            JsonValue root;
            std::string parserError;

            JsonParser parser(contents);

            if (!parser.Parse(root, parserError))
            {
                error =
                    L"Level JSON is malformed.";

                return false;
            }

            std::string format;
            std::string name;
            std::string guid;

            std::uint64_t version = 0U;
            std::uint64_t nextEntityId = 0U;

            if (
                !ReadString(
                    RequireField(root, "format"),
                    format) ||
                !ReadUnsigned(
                    RequireField(root, "version"),
                    version) ||
                !ReadString(
                    RequireField(root, "name"),
                    name) ||
                !ReadString(
                    RequireField(root, "guid"),
                    guid) ||
                !ReadUnsigned(
                    RequireField(
                        root,
                        "nextEntityId"),
                    nextEntityId) ||
                format != FormatName ||
                (
                    version != LegacyFormatVersion &&
                    version != ComponentFormatVersion &&
                    version != SingleMeshCharacterFormatVersion &&
                    version != ModularCharacterFormatVersion &&
                    version != InlineCharacterAnimationFormatVersion &&
                    version != CurrentFormatVersion
                ) ||
                !FromUtf8(name, data.name) ||
                !FromUtf8(guid, data.guid))
            {
                error =
                    L"The level format or metadata is invalid.";

                return false;
            }

            data.snapshot = {};

            data.snapshot.nextEntityId =
                static_cast<EditorEntityId>(
                    nextEntityId);

            data.snapshot.selectedIndex =
                InvalidEditorEntityIndex;

            data.snapshot.dirty = false;

            const JsonValue* const selected =
                RequireField(
                    root,
                    "selectedIndex");

            if (selected == nullptr)
            {
                error =
                    L"selectedIndex is missing.";

                return false;
            }

            if (
                selected->type !=
                JsonValue::Type::Null)
            {
                std::uint64_t selectedIndex = 0U;

                if (!ReadUnsigned(
                        selected,
                        selectedIndex) ||
                    selectedIndex >
                        static_cast<std::uint64_t>(
                            std::numeric_limits<
                                std::size_t>::max()))
                {
                    error =
                        L"selectedIndex is invalid.";

                    return false;
                }

                data.snapshot.selectedIndex =
                    static_cast<std::size_t>(
                        selectedIndex);
            }

            const JsonValue* const entities =
                RequireField(
                    root,
                    "entities");

            if (
                entities == nullptr ||
                entities->type !=
                    JsonValue::Type::Array)
            {
                error =
                    L"The entities array is missing.";

                return false;
            }

            data.snapshot.entities.clear();

            data.snapshot.entities.reserve(
                entities->array.size());

            for (
                const JsonValue& jsonEntity :
                entities->array)
            {
                EditorSceneEntity entity;

                const bool parsed =
                    version ==
                        LegacyFormatVersion
                        ? ParseLegacyEntity(
                            jsonEntity,
                            entity)
                        : ParseComponentEntity(
                            jsonEntity,
                            entity);

                if (!parsed)
                {
                    error =
                        L"An entity record is malformed.";

                    return false;
                }

                data.snapshot.entities.push_back(
                    std::move(entity));
            }

            return Validate(data, error);
        }

        [[nodiscard]]
        bool ResolveCharacterAnimationProfilePath(
            const std::filesystem::path& profilePath,
            std::filesystem::path& resolvedPath,
            std::wstring& error)
        {
            resolvedPath.clear();

            if (profilePath.empty())
            {
                error =
                    L"Character animation profile "
                    L"path is empty.";

                return false;
            }

            const auto isRegularFile =
                [](
                    const std::filesystem::path&
                        path) noexcept
                {
                    std::error_code filesystemError;

                    return
                        std::filesystem::is_regular_file(
                            path,
                            filesystemError) &&
                        !filesystemError;
                };

            if (
                profilePath.is_absolute() &&
                isRegularFile(profilePath))
            {
                resolvedPath =
                    profilePath.lexically_normal();

                return true;
            }

            std::error_code filesystemError;

            std::filesystem::path current =
                std::filesystem::current_path(
                    filesystemError);

            if (filesystemError)
            {
                error =
                    L"Failed to resolve the current "
                    L"working directory.";

                return false;
            }

            current =
                current.lexically_normal();

            while (!current.empty())
            {
                const std::filesystem::path direct =
                    (
                        current /
                        profilePath
                    ).lexically_normal();

                if (isRegularFile(direct))
                {
                    resolvedPath = direct;
                    return true;
                }

                const std::filesystem::path gamePath =
                    (
                        current /
                        L"game" /
                        profilePath
                    ).lexically_normal();

                if (isRegularFile(gamePath))
                {
                    resolvedPath = gamePath;
                    return true;
                }

                const std::filesystem::path parent =
                    current.parent_path();

                if (
                    parent.empty() ||
                    parent == current)
                {
                    break;
                }

                current = parent;
            }

            error =
                L"Character animation profile "
                L"was not found: ";

            error +=
                profilePath.generic_wstring();

            return false;
        }

        [[nodiscard]]
        bool ReadFile(
            const std::filesystem::path& path,
            std::string& contents,
            std::wstring& error)
        {
            std::error_code filesystemError;

            const std::uintmax_t size =
                std::filesystem::file_size(
                    path,
                    filesystemError);

            if (
                filesystemError ||
                size > MaximumLevelFileSize)
            {
                error =
                    L"The level file is inaccessible or too large.";

                return false;
            }

            std::ifstream input(
                path,
                std::ios::binary);

            if (!input)
            {
                error =
                    L"Failed to open the level file.";

                return false;
            }

            contents.resize(
                static_cast<std::size_t>(size));

            if (
                size > 0U &&
                !input.read(
                    contents.data(),
                    static_cast<std::streamsize>(
                        size)))
            {
                error =
                    L"Failed to read the complete level file.";

                return false;
            }

            return true;
        }
    }

    bool LevelSerializer::Save(
        const std::filesystem::path& path,
        const EditorLevelFileData& data,
        std::wstring& error)
    {
        error.clear();

        if (path.empty())
        {
            error =
                L"The target level path is empty.";

            return false;
        }

        if (!Validate(data, error))
        {
            return false;
        }

        std::string json;

        if (!BuildJson(data, json, error))
        {
            return false;
        }

        std::error_code filesystemError;

        if (!path.parent_path().empty())
        {
            std::filesystem::create_directories(
                path.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    L"Failed to create the level directory.";

                return false;
            }
        }

        std::filesystem::path temporaryPath =
            path;

        temporaryPath += L".tmp";

        {
            std::ofstream output(
                temporaryPath,
                std::ios::binary |
                    std::ios::trunc);

            if (!output)
            {
                error =
                    L"Failed to create the temporary level file.";

                return false;
            }

            output.write(
                json.data(),
                static_cast<std::streamsize>(
                    json.size()));

            output.flush();

            if (!output.good())
            {
                error =
                    L"Failed to write the temporary level file.";

                output.close();

                std::filesystem::remove(
                    temporaryPath,
                    filesystemError);

                return false;
            }
        }

        if (!MoveFileExW(
                temporaryPath.c_str(),
                path.c_str(),
                MOVEFILE_REPLACE_EXISTING |
                    MOVEFILE_WRITE_THROUGH))
        {
            error =
                L"Failed to replace the target level file. Win32 error: ";

            error += std::to_wstring(
                GetLastError());

            std::filesystem::remove(
                temporaryPath,
                filesystemError);

            return false;
        }

        return true;
    }

    bool LevelSerializer::Load(
        const std::filesystem::path& path,
        EditorLevelFileData& data,
        std::wstring& error)
    {
        error.clear();

        std::string contents;

        if (!ReadFile(
                path,
                contents,
                error))
        {
            return false;
        }

        return ParseJson(
            contents,
            data,
            error);
    }

    bool LevelSerializer::
        LoadCharacterAnimationProfile(
            const std::filesystem::path& profilePath,
            engine::scene::
                CharacterAnimationSet& animationSet,
            std::wstring& error)
    {
        error.clear();

        std::filesystem::path resolvedPath;

        if (!ResolveCharacterAnimationProfilePath(
                profilePath,
                resolvedPath,
                error))
        {
            return false;
        }

        std::string contents;

        if (!ReadFile(
                resolvedPath,
                contents,
                error))
        {
            return false;
        }

        /*
         * Поддерживаем UTF-8 BOM.
         */
        if (
            contents.size() >= 3U &&
            static_cast<unsigned char>(
                contents[0U]) == 0xEFU &&
            static_cast<unsigned char>(
                contents[1U]) == 0xBBU &&
            static_cast<unsigned char>(
                contents[2U]) == 0xBFU)
        {
            contents.erase(
                0U,
                3U);
        }

        JsonValue root;
        std::string parserError;

        JsonParser parser(contents);

        if (!parser.Parse(
                root,
                parserError))
        {
            error =
                L"Character animation profile "
                L"JSON is malformed.";

            return false;
        }

        std::string format;
        std::uint64_t version = 0U;

        if (
            !ReadString(
                RequireField(
                    root,
                    "format"),
                format) ||
            !ReadUnsigned(
                RequireField(
                    root,
                    "version"),
                version) ||
            format !=
                CharacterAnimationProfileFormatName ||
            version !=
                CharacterAnimationProfileFormatVersion)
        {
            error =
                L"Character animation profile "
                L"format or version is invalid.";

            return false;
        }

        const JsonValue* const animationSetJson =
            RequireField(
                root,
                "animationSet");

        if (
            animationSetJson == nullptr ||
            animationSetJson->type !=
                JsonValue::Type::Object)
        {
            error =
                L"Character animation profile "
                L"does not contain animationSet.";

            return false;
        }

        engine::scene::CharacterAnimationSet
            loadedSet;

        if (!ReadCharacterAnimationSet(
                *animationSetJson,
                loadedSet))
        {
            error =
                L"Character animation profile "
                L"contains invalid fields.";

            return false;
        }

        if (
            loadedSet.upperBodyRootBone.empty() ||
            loadedSet.actionRootBone.empty() ||
            loadedSet.lookRootBone.empty())
        {
            error =
                L"Character animation profile "
                L"contains an empty bone name.";

            return false;
        }

        animationSet =
            std::move(loadedSet);

        return true;
    }
}
