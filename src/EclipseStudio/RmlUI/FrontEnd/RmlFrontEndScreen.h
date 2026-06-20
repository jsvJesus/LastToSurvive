#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/EventListener.h>

#include <functional>
#include <memory>

class RmlFrontEndScreen
{
public:
    RmlFrontEndScreen();
    virtual ~RmlFrontEndScreen();

    bool Load(
        Rml::Context* InContext,
        const char* PrimaryDocumentPath,
        const char* FallbackDocumentPath = nullptr
    );

    void Unload();

    void Show();
    void Hide();

    bool IsLoaded() const;

    Rml::ElementDocument* GetDocument() const;

    void SetClickHandler(
        const std::function<void(const Rml::String&)>& InClickHandler
    );

protected:
    virtual void OnDocumentLoaded();
    virtual void OnDocumentUnloaded();

    virtual bool HandleClickId(
        const Rml::String& Id
    );

    void SetElementText(
        const char* ElementId,
        const Rml::String& Text
    );

    void SetElementClass(
        const char* ElementId,
        const char* ClassName,
        bool bEnabled
    );

    void SetElementProperty(
        const char* ElementId,
        const char* PropertyName,
        const Rml::String& Value
    );

    void SetElementAttribute(
        const char* ElementId,
        const char* AttributeName,
        const Rml::String& Value
    );

private:
    class FClickListener final :
        public Rml::EventListener
    {
    public:
        explicit FClickListener(
            RmlFrontEndScreen* InOwner
        );

        void ProcessEvent(
            Rml::Event& Event
        ) override;

        void OnDetach(
            Rml::Element* Element
        ) override;

    private:
        RmlFrontEndScreen* Owner = nullptr;
    };

private:
    void ProcessClick(
        Rml::Element* Element
    );

protected:
    Rml::Context* Context = nullptr;
    Rml::ElementDocument* Document = nullptr;

private:
    std::unique_ptr<FClickListener> ClickListener;
    std::function<void(const Rml::String&)> ClickHandler;
};