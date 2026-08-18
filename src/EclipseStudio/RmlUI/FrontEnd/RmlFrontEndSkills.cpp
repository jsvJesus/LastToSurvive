#include "RmlFrontEndSkills.h"

#include <cstddef>

namespace
{
    constexpr char SkillNodeButtonPrefix[] =
        "skill_node_";

    constexpr std::size_t SkillNodeButtonPrefixLength =
        sizeof(SkillNodeButtonPrefix) - 1;
}

RmlFrontEndSkills::RmlFrontEndSkills()
{
}

RmlFrontEndSkills::~RmlFrontEndSkills()
{
}

bool RmlFrontEndSkills::Load(
    Rml::Context* InContext
)
{
    return RmlFrontEndScreen::Load(
        InContext,

        "Rml/FrontEnd/Skills.rml"
    );
}

void RmlFrontEndSkills::SetCallbacks(
    const FRmlFrontEndSkillsCallbacks& InCallbacks
)
{
    Callbacks =
        InCallbacks;
}

void RmlFrontEndSkills::Refresh()
{
    if (Callbacks.BuildSkills)
    {
        Callbacks.BuildSkills();
    }
}

bool RmlFrontEndSkills::HandleClickId(
    const Rml::String& Id
)
{
    if (Id == "nav_survivor" ||
        Id == "btn_back_to_main" ||
        Id == "btn_skills_back")
    {
        if (Callbacks.ShowMainMenu)
            Callbacks.ShowMainMenu();

        return true;
    }

    if (Id == "nav_shop")
    {
        if (Callbacks.ShowShop)
            Callbacks.ShowShop();

        return true;
    }

    if (Id == "nav_skills")
    {
        return true;
    }

    if (Id == "btn_learn_selected_skill")
    {
        if (Callbacks.RequestLearnSelectedSkill)
            Callbacks.RequestLearnSelectedSkill();

        return true;
    }

    if (Id == "btn_reset_skills")
    {
        if (Callbacks.ResetSkills)
            Callbacks.ResetSkills();

        return true;
    }

    if (IsSkillNodeId(Id))
    {
        if (Callbacks.SelectSkillNode)
            Callbacks.SelectSkillNode(Id);

        return true;
    }

    if (IsSkillCategoryId(Id))
    {
        if (Callbacks.SelectSkillCategory)
            Callbacks.SelectSkillCategory(Id);

        return true;
    }

    return false;
}

bool RmlFrontEndSkills::IsSkillNodeId(
    const Rml::String& Id
) const
{
    return Id.compare(
        0,
        SkillNodeButtonPrefixLength,
        SkillNodeButtonPrefix
    ) == 0;
}

bool RmlFrontEndSkills::IsSkillCategoryId(
    const Rml::String& Id
) const
{
    return
        Id == "skill_category_survival" ||
        Id == "skill_category_combat" ||
        Id == "skill_category_support" ||
        Id == "skill_category_crafting";
}
