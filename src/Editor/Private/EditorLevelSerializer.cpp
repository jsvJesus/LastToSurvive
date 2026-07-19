#include "Editor/EditorLevelSerializer.h"

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
        constexpr std::uint64_t CurrentFormatVersion = 2U;
        constexpr std::uint64_t LegacyFormatVersion = 1U;

        constexpr std::string_view FormatName =
            "LTS.Level";

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
        const JsonValue* RequireField(
            const JsonValue& object,
            const std::string_view name) noexcept
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

            if (
                const JsonValue* component =
                    RequireField(
                        *components,
                        "Environment"))
            {
                engine::scene::EnvironmentComponent
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
                        value.environmentAsset))
                {
                    return false;
                }

                entity.environment =
                    std::move(value);
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
                    std::string asset;

                    if (!ToUtf8(
                            entity.environment->
                                environmentAsset,
                            asset))
                    {
                        error =
                            L"Failed to convert an environment asset path.";

                        return false;
                    }

                    output
                        << ",\n"
                        << "        \"Environment\": {\"asset\": ";

                    WriteJsonString(output, asset);

                    output << '}';
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

    bool EditorLevelSerializer::Save(
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

    bool EditorLevelSerializer::Load(
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
}
