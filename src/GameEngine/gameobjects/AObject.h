#pragma once

#include <string>
#include <vector>

class AObject;

using stringlist_t =
    std::vector<std::string>;

using AObjectConstructor =
    AObject* (*)();

#define DECLARE_BASE_CLASS(TClass, TParent)             \
public:                                                 \
    using parent = TParent;                             \
                                                        \
    static AObject* ClassConstructor()                  \
    {                                                   \
        return new TClass;                              \
    }                                                   \
                                                        \
public:

#define DECLARE_CLASS(TClass, TParent)                  \
    DECLARE_BASE_CLASS(TClass, TParent)                 \
    static AClass ClassData;                            \
                                                        \
public:

#define IMPLEMENT_CLASS(TClass, name, type)             \
    AClass TClass::ClassData                            \
    (                                                   \
        &TClass::parent::ClassData,                     \
        name,                                           \
        type,                                           \
        &TClass::ClassConstructor                       \
    );                                                  \
                                                        \
    extern "C" AClass* classdata##TClass;               \
    AClass* classdata##TClass =                         \
        &TClass::ClassData;

#define AUTOREGISTER_CLASS(TClass)                      \
    static int REG_##TClass()                           \
    {                                                   \
        return AObjectTable_RegisterClass(              \
            classdata##TClass);                         \
    }                                                   \
                                                        \
    static int REG_##TClass##Init =                     \
        REG_##TClass();

class AClass
{
public:
    AClass(
        AClass* baseClass,
        const char* name,
        const char* type,
        AObjectConstructor constructor) noexcept
        : ID(-1),
          Name(name != nullptr ? name : ""),
          Type(type != nullptr ? type : ""),
          BaseClass(baseClass),
          ClassConstructor(constructor)
    {
    }

public:
    int ID;

    std::string Name;
    std::string Type;

    AClass* BaseClass;

    AObjectConstructor
        ClassConstructor;
};

class AObject
{
public:
    DECLARE_CLASS(AObject, AObject)

    AClass* Class = nullptr;
};

extern int AObjectTable_RegisterClass(
    AClass* objectClass);

extern int AObjectTable_GetClassID(
    const char* name,
    const char* type);

extern AObject* AObjectTable_CreateObject(
    int id);

extern stringlist_t
    AObjectTable_GetRegisteredClassesList();