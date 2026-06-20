#pragma once

#include "RmlFrontEndScreen.h"

#include <functional>

struct FRmlFrontEndSkillsCallbacks
{
    std::function<void()> ShowMainMenu;
    std::function<void()> BuildSkills;
    std::function<void()> RequestLearnSelectedSkill;
    std::function<void()> ResetSkills;

    std::function<void(const Rml::String&)> SelectSkillNode;
    std::function<void(const Rml::String&)> SelectSkillCategory;
    std::function<void(const Rml::String&)> SetSkillsStatus;
};

class RmlFrontEndSkills final :
    public RmlFrontEndScreen
{
public:
    RmlFrontEndSkills();
    ~RmlFrontEndSkills() override;

    bool Load(
        Rml::Context* InContext
    );

    void SetCallbacks(
        const FRmlFrontEndSkillsCallbacks& InCallbacks
    );

    void Refresh();

protected:
    bool HandleClickId(
        const Rml::String& Id
    ) override;

private:
    bool IsSkillNodeId(
        const Rml::String& Id
    ) const;

    bool IsSkillCategoryId(
        const Rml::String& Id
    ) const;

private:
    FRmlFrontEndSkillsCallbacks Callbacks;
};