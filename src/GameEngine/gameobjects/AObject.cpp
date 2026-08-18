#include <Core/Log.h>

#include "AObject.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string_view>

IMPLEMENT_CLASS(
    AObject,
    "AObject",
    "Core")

namespace
{
    constexpr int MaxRegisteredObjects =
        1024;

    void ReportRegistryCapacityExceeded() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Error,
            "GameObject.Registry",
            "Maximum registered object class "
            "count reached");

    }

    [[noreturn]]
    void FailInvalidClassId(
        const int id) noexcept
    {
        std::array<char, 128> message{};

        const int written =
            std::snprintf(
                message.data(),
                message.size(),
                "Object class ID %d is not registered",
                id);

        if (written > 0)
        {
            const std::size_t length =
                static_cast<std::size_t>(
                    written) <
                        message.size()
                    ? static_cast<std::size_t>(
                        written)
                    : message.size() - 1;

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Critical,
                "GameObject.Registry",
                std::string_view(
                    message.data(),
                    length));
        }
        else
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Critical,
                "GameObject.Registry",
                "Invalid object class ID");
        }

        std::abort();

    }
}

class AObjectTable
{
private:
    struct Entry
    {
        int ID = -1;
        AClass* Class = nullptr;
    };

private:
    static AObjectTable* instance_;

    AObjectTable() noexcept;
    ~AObjectTable() noexcept;

public:
    static AObjectTable* Get();

    int RegisterClass(
        AClass* objectClass);

    int GetClassID(
        const char* name,
        const char* type) const noexcept;

    AObject* CreateObject(
        int id);

    stringlist_t
        GetRegisteredClassesList() const;

private:
    Entry table_[MaxRegisteredObjects]{};

    int entryCount_ = 0;
};

AObjectTable*
    AObjectTable::instance_ =
        nullptr;

AObjectTable::AObjectTable() noexcept = default;

AObjectTable::~AObjectTable() noexcept
{
    entryCount_ = 0;
}

AObjectTable* AObjectTable::Get()
{
    if (instance_ == nullptr)
    {
        instance_ =
            new AObjectTable;
    }

    return instance_;
}

int AObjectTable::RegisterClass(
    AClass* const objectClass)
{
    if (objectClass == nullptr)
    {
        return 0;
    }

    if (
        entryCount_ + 1 >=
        MaxRegisteredObjects
    )
    {
        ReportRegistryCapacityExceeded();

        return 0;
    }

    Entry& entry =
        table_[entryCount_];

    entry.ID =
        entryCount_;

    entry.Class =
        objectClass;

    objectClass->ID =
        entryCount_;

    ++entryCount_;

    return 1;
}

AObject* AObjectTable::CreateObject(
    const int id)
{
    if (
        id < 0 ||
        id >= entryCount_ ||
        table_[id].ID != id ||
        table_[id].Class == nullptr ||
        table_[id].Class->
            ClassConstructor == nullptr
    )
    {
        FailInvalidClassId(id);
    }

    AClass* const objectClass =
        table_[id].Class;

    AObject* const object =
        objectClass->
            ClassConstructor();

    if (object == nullptr)
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Critical,
            "GameObject.Registry",
            "Object class constructor "
            "returned null");

        std::abort();

    }

    object->Class =
        objectClass;

    return object;
}

int AObjectTable::GetClassID(
    const char* const name,
    const char* const type) const noexcept
{
    if (
        name == nullptr ||
        type == nullptr
    )
    {
        return -1;
    }

    for (
        int index = 0;
        index < entryCount_;
        ++index
    )
    {
        const AClass* const objectClass =
            table_[index].Class;

        if (objectClass == nullptr)
        {
            continue;
        }

        if (
            objectClass->Name == name &&
            objectClass->Type == type
        )
        {
            return index;
        }
    }

    return -1;
}

stringlist_t
    AObjectTable::
        GetRegisteredClassesList() const
{
    stringlist_t list;

    list.reserve(
        static_cast<std::size_t>(
            entryCount_));

    for (
        int index = 0;
        index < entryCount_;
        ++index
    )
    {
        const AClass* const objectClass =
            table_[index].Class;

        if (objectClass == nullptr)
        {
            continue;
        }

        list.push_back(
            objectClass->Name);
    }

    return list;
}

int AObjectTable_RegisterClass(
    AClass* const objectClass)
{
    return
        AObjectTable::Get()->
            RegisterClass(
                objectClass);
}

int AObjectTable_GetClassID(
    const char* const name,
    const char* const type)
{
    return
        AObjectTable::Get()->
            GetClassID(
                name,
                type);
}

AObject* AObjectTable_CreateObject(
    const int id)
{
    return
        AObjectTable::Get()->
            CreateObject(
                id);
}

stringlist_t
    AObjectTable_GetRegisteredClassesList()
{
    return
        AObjectTable::Get()->
            GetRegisteredClassesList();
}
