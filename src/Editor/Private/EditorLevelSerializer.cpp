#include "Editor/EditorLevelSerializer.h"

#include <Windows.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint64_t CurrentFormatVersion = 1U;
        constexpr std::string_view FormatName = "LTS.Level";

        constexpr std::uintmax_t MaximumLevelFileSize =
            64U * 1024U * 1024U;

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

            const int required = WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                input.data(),
                static_cast<int>(input.size()),
                nullptr,
                0,
                nullptr,
                nullptr);

            if (required <= 0)
            {
                return false;
            }

            output.resize(
                static_cast<std::size_t>(required));

            return WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                input.data(),
                static_cast<int>(input.size()),
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

            const int required = MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                input.data(),
                static_cast<int>(input.size()),
                nullptr,
                0);

            if (required <= 0)
            {
                return false;
            }

            output.resize(
                static_cast<std::size_t>(required));

            return MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                input.data(),
                static_cast<int>(input.size()),
                output.data(),
                required) == required;
        }

        void WriteJsonString(
            std::ostream& output,
            const std::string_view value)
        {
            static constexpr char Hex[] =
                "0123456789ABCDEF";

            output.put('"');

            for (const unsigned char character : value)
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
                        if (character < 0x20U)
                        {
                            output
                                << "\\u00"
                                << Hex[
                                    (character >> 4U) &
                                    0x0FU]
                                << Hex[
                                    character &
                                    0x0FU];
                        }
                        else
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
            const std::string_view text,
            EditorEntityKind& kind) noexcept
        {
            if (text == "Environment")
            {
                kind = EditorEntityKind::Environment;
            }
            else if (text == "DirectionalLight")
            {
                kind =
                    EditorEntityKind::DirectionalLight;
            }
            else if (text == "SpawnPoint")
            {
                kind = EditorEntityKind::SpawnPoint;
            }
            else if (text == "Anomaly")
            {
                kind = EditorEntityKind::Anomaly;
            }
            else if (text == "LootContainer")
            {
                kind =
                    EditorEntityKind::LootContainer;
            }
            else if (text == "Empty")
            {
                kind = EditorEntityKind::Empty;
            }
            else
            {
                return false;
            }

            return true;
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

            output
                << std::setprecision(9);

            output << "{\n";
            output << "  \"format\": ";

            WriteJsonString(
                output,
                FormatName);

            output
                << ",\n"
                << "  \"version\": "
                << CurrentFormatVersion
                << ",\n"
                << "  \"name\": ";

            WriteJsonString(
                output,
                levelName);

            output
                << ",\n"
                << "  \"guid\": ";

            WriteJsonString(
                output,
                levelGuid);

            output
                << ",\n"
                << "  \"nextEntityId\": "
                << data.snapshot.nextEntityId
                << ",\n"
                << "  \"selectedIndex\": ";

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

            output
                << ",\n"
                << "  \"entities\": [\n";

            for (
                std::size_t index = 0U;
                index <
                    data.snapshot.entities.size();
                ++index)
            {
                const EditorSceneEntity& entity =
                    data.snapshot.entities[index];

                std::string entityName;

                if (!ToUtf8(
                        entity.name,
                        entityName))
                {
                    error =
                        L"Failed to convert an entity name to UTF-8.";

                    return false;
                }

                output
                    << "    {\"id\": "
                    << entity.id
                    << ", \"kind\": ";

                WriteJsonString(
                    output,
                    KindToString(entity.kind));

                output << ", \"name\": ";

                WriteJsonString(
                    output,
                    entityName);

                output
                    << ", \"position\": ";

                WriteVector3(
                    output,
                    entity.transform.position);

                output
                    << ", \"rotation\": ";

                WriteVector3(
                    output,
                    entity.transform.rotationDegrees);

                output
                    << ", \"scale\": ";

                WriteVector3(
                    output,
                    entity.transform.scale);

                output << '}';

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

        void SkipWhitespace(
            const std::string_view text,
            std::size_t& position) noexcept
        {
            while (position < text.size())
            {
                const char value =
                    text[position];

                if (
                    value != ' ' &&
                    value != '\t' &&
                    value != '\r' &&
                    value != '\n')
                {
                    break;
                }

                ++position;
            }
        }

        [[nodiscard]]
        bool FindField(
            const std::string_view text,
            const std::string_view field,
            std::size_t& valuePosition)
        {
            std::string key;

            key.reserve(
                field.size() + 2U);

            key.push_back('"');
            key.append(field);
            key.push_back('"');

            const std::size_t keyPosition =
                text.find(key);

            if (
                keyPosition ==
                std::string_view::npos)
            {
                return false;
            }

            const std::size_t colon =
                text.find(
                    ':',
                    keyPosition + key.size());

            if (
                colon ==
                std::string_view::npos)
            {
                return false;
            }

            valuePosition = colon + 1U;

            SkipWhitespace(
                text,
                valuePosition);

            return
                valuePosition <
                text.size();
        }

        [[nodiscard]]
        bool ReadJsonString(
            const std::string_view text,
            std::size_t& position,
            std::string& value)
        {
            SkipWhitespace(
                text,
                position);

            if (
                position >= text.size() ||
                text[position] != '"')
            {
                return false;
            }

            ++position;
            value.clear();

            while (position < text.size())
            {
                const char character =
                    text[position++];

                if (character == '"')
                {
                    return true;
                }

                if (
                    static_cast<unsigned char>(
                        character) < 0x20U)
                {
                    return false;
                }

                if (character != '\\')
                {
                    value.push_back(character);
                    continue;
                }

                if (position >= text.size())
                {
                    return false;
                }

                switch (text[position++])
                {
                    case '"':
                        value.push_back('"');
                        break;

                    case '\\':
                        value.push_back('\\');
                        break;

                    case '/':
                        value.push_back('/');
                        break;

                    case 'b':
                        value.push_back('\b');
                        break;

                    case 'f':
                        value.push_back('\f');
                        break;

                    case 'n':
                        value.push_back('\n');
                        break;

                    case 'r':
                        value.push_back('\r');
                        break;

                    case 't':
                        value.push_back('\t');
                        break;

                    default:
                        return false;
                }
            }

            return false;
        }

        [[nodiscard]]
        bool ReadUnsigned(
            const std::string_view text,
            std::size_t& position,
            std::uint64_t& value)
        {
            SkipWhitespace(
                text,
                position);

            const std::size_t start =
                position;

            while (
                position < text.size() &&
                text[position] >= '0' &&
                text[position] <= '9')
            {
                ++position;
            }

            if (start == position)
            {
                return false;
            }

            const auto result =
                std::from_chars(
                    text.data() + start,
                    text.data() + position,
                    value);

            return
                result.ec == std::errc{} &&
                result.ptr ==
                    text.data() + position;
        }

        [[nodiscard]]
        bool ReadFloat(
            const std::string_view text,
            std::size_t& position,
            float& value)
        {
            SkipWhitespace(
                text,
                position);

            const std::size_t start =
                position;

            if (
                position < text.size() &&
                text[position] == '-')
            {
                ++position;
            }

            while (
                position < text.size() &&
                (
                    (
                        text[position] >= '0' &&
                        text[position] <= '9'
                    ) ||
                    text[position] == '.' ||
                    text[position] == 'e' ||
                    text[position] == 'E' ||
                    text[position] == '+' ||
                    text[position] == '-'
                ))
            {
                ++position;
            }

            if (start == position)
            {
                return false;
            }

            const std::string token(
                text.substr(
                    start,
                    position - start));

            char* end = nullptr;

            const float parsed =
                std::strtof(
                    token.c_str(),
                    &end);

            if (
                end == nullptr ||
                end == token.c_str() ||
                *end != '\0' ||
                !std::isfinite(parsed))
            {
                return false;
            }

            value = parsed;
            return true;
        }

        [[nodiscard]]
        bool ReadVector3(
            const std::string_view text,
            std::size_t& position,
            std::array<float, 3U>& value)
        {
            SkipWhitespace(
                text,
                position);

            if (
                position >= text.size() ||
                text[position++] != '[')
            {
                return false;
            }

            for (
                std::size_t index = 0U;
                index < value.size();
                ++index)
            {
                if (!ReadFloat(
                        text,
                        position,
                        value[index]))
                {
                    return false;
                }

                SkipWhitespace(
                    text,
                    position);

                if (index + 1U < value.size())
                {
                    if (
                        position >= text.size() ||
                        text[position++] != ',')
                    {
                        return false;
                    }
                }
            }

            SkipWhitespace(
                text,
                position);

            return
                position < text.size() &&
                text[position++] == ']';
        }

        [[nodiscard]]
        bool ReadStringField(
            const std::string_view object,
            const std::string_view field,
            std::string& value)
        {
            std::size_t position = 0U;

            return
                FindField(
                    object,
                    field,
                    position) &&
                ReadJsonString(
                    object,
                    position,
                    value);
        }

        [[nodiscard]]
        bool ReadUnsignedField(
            const std::string_view object,
            const std::string_view field,
            std::uint64_t& value)
        {
            std::size_t position = 0U;

            return
                FindField(
                    object,
                    field,
                    position) &&
                ReadUnsigned(
                    object,
                    position,
                    value);
        }

        [[nodiscard]]
        bool ReadVectorField(
            const std::string_view object,
            const std::string_view field,
            std::array<float, 3U>& value)
        {
            std::size_t position = 0U;

            return
                FindField(
                    object,
                    field,
                    position) &&
                ReadVector3(
                    object,
                    position,
                    value);
        }

        [[nodiscard]]
        bool FindMatching(
            const std::string_view text,
            const std::size_t openingPosition,
            const char opening,
            const char closing,
            std::size_t& closingPosition)
        {
            int depth = 0;
            bool inString = false;
            bool escaped = false;

            for (
                std::size_t index =
                    openingPosition;
                index < text.size();
                ++index)
            {
                const char value =
                    text[index];

                if (inString)
                {
                    if (escaped)
                    {
                        escaped = false;
                    }
                    else if (value == '\\')
                    {
                        escaped = true;
                    }
                    else if (value == '"')
                    {
                        inString = false;
                    }

                    continue;
                }

                if (value == '"')
                {
                    inString = true;
                    continue;
                }

                if (value == opening)
                {
                    ++depth;
                }
                else if (
                    value == closing &&
                    --depth == 0)
                {
                    closingPosition = index;
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        bool ExtractEntityObjects(
            const std::string_view document,
            std::vector<std::string_view>& objects)
        {
            std::size_t valuePosition = 0U;

            if (
                !FindField(
                    document,
                    "entities",
                    valuePosition) ||
                valuePosition >=
                    document.size() ||
                document[valuePosition] != '[')
            {
                return false;
            }

            std::size_t arrayEnd = 0U;

            if (!FindMatching(
                    document,
                    valuePosition,
                    '[',
                    ']',
                    arrayEnd))
            {
                return false;
            }

            objects.clear();

            std::size_t position =
                valuePosition + 1U;

            while (position < arrayEnd)
            {
                SkipWhitespace(
                    document,
                    position);

                if (
                    position < arrayEnd &&
                    document[position] == ',')
                {
                    ++position;
                    continue;
                }

                if (position >= arrayEnd)
                {
                    break;
                }

                if (document[position] != '{')
                {
                    return false;
                }

                std::size_t objectEnd = 0U;

                if (
                    !FindMatching(
                        document,
                        position,
                        '{',
                        '}',
                        objectEnd) ||
                    objectEnd > arrayEnd)
                {
                    return false;
                }

                objects.push_back(
                    document.substr(
                        position,
                        objectEnd -
                            position +
                            1U));

                position = objectEnd + 1U;
            }

            return true;
        }

        [[nodiscard]]
        bool ParseEntity(
            const std::string_view object,
            EditorSceneEntity& entity)
        {
            std::uint64_t id = 0U;
            std::string kind;
            std::string name;

            if (
                !ReadUnsignedField(
                    object,
                    "id",
                    id) ||
                !ReadStringField(
                    object,
                    "kind",
                    kind) ||
                !ReadStringField(
                    object,
                    "name",
                    name) ||
                !ReadVectorField(
                    object,
                    "position",
                    entity.transform.position) ||
                !ReadVectorField(
                    object,
                    "rotation",
                    entity.transform.rotationDegrees) ||
                !ReadVectorField(
                    object,
                    "scale",
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

            std::unordered_set<EditorEntityId>
                ids;

            EditorEntityId maximumId = 0U;

            for (
                const EditorSceneEntity& entity :
                data.snapshot.entities)
            {
                if (
                    entity.id == 0U ||
                    !ids.insert(entity.id).second ||
                    entity.name.empty())
                {
                    error =
                        L"Entity IDs or names are invalid.";

                    return false;
                }

                maximumId =
                    std::max(
                        maximumId,
                        entity.id);

                for (
                    const float value :
                    entity.transform.position)
                {
                    if (!std::isfinite(value))
                    {
                        error =
                            L"An entity position is invalid.";

                        return false;
                    }
                }

                for (
                    const float value :
                    entity.transform.rotationDegrees)
                {
                    if (!std::isfinite(value))
                    {
                        error =
                            L"An entity rotation is invalid.";

                        return false;
                    }
                }

                for (
                    const float value :
                    entity.transform.scale)
                {
                    if (
                        !std::isfinite(value) ||
                        std::abs(value) < 0.001F)
                    {
                        error =
                            L"An entity scale is invalid.";

                        return false;
                    }
                }
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
        bool ParseJson(
            const std::string_view document,
            EditorLevelFileData& data,
            std::wstring& error)
        {
            std::string format;
            std::string name;
            std::string guid;

            std::uint64_t version = 0U;
            std::uint64_t nextEntityId = 0U;

            if (
                !ReadStringField(
                    document,
                    "format",
                    format) ||
                !ReadUnsignedField(
                    document,
                    "version",
                    version) ||
                !ReadStringField(
                    document,
                    "name",
                    name) ||
                !ReadStringField(
                    document,
                    "guid",
                    guid) ||
                !ReadUnsignedField(
                    document,
                    "nextEntityId",
                    nextEntityId))
            {
                error =
                    L"Required level fields are missing or malformed.";

                return false;
            }

            if (
                format != FormatName ||
                version != CurrentFormatVersion)
            {
                error =
                    L"The level format or version is not supported.";

                return false;
            }

            if (
                !FromUtf8(
                    name,
                    data.name) ||
                !FromUtf8(
                    guid,
                    data.guid))
            {
                error =
                    L"Level metadata is not valid UTF-8.";

                return false;
            }

            data.snapshot.nextEntityId =
                static_cast<EditorEntityId>(
                    nextEntityId);

            data.snapshot.selectedIndex =
                InvalidEditorEntityIndex;

            data.snapshot.dirty = false;

            std::size_t selectedPosition = 0U;

            if (!FindField(
                    document,
                    "selectedIndex",
                    selectedPosition))
            {
                error =
                    L"selectedIndex is missing.";

                return false;
            }

            if (
                document.substr(
                    selectedPosition,
                    4U) != "null")
            {
                std::uint64_t selected = 0U;

                if (
                    !ReadUnsigned(
                        document,
                        selectedPosition,
                        selected) ||
                    selected >
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
                        selected);
            }

            std::vector<std::string_view>
                entityObjects;

            if (!ExtractEntityObjects(
                    document,
                    entityObjects))
            {
                error =
                    L"The entities array is malformed.";

                return false;
            }

            data.snapshot.entities.clear();

            data.snapshot.entities.reserve(
                entityObjects.size());

            for (
                const std::string_view object :
                entityObjects)
            {
                EditorSceneEntity entity;

                if (!ParseEntity(
                        object,
                        entity))
                {
                    error =
                        L"An entity record is malformed.";

                    return false;
                }

                data.snapshot.entities.push_back(
                    std::move(entity));
            }

            return Validate(
                data,
                error);
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
                    L"The level file is missing, inaccessible, or too large.";

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

        if (!BuildJson(
                data,
                json,
                error))
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