#pragma once

//
// Temporary UI compatibility types.
//
// Scaleform is removed.
// This value replaces old Scaleform::GFx::Value for dead HUD paths.
//

struct HUDNullValue
{
    HUDNullValue()
        : bUndefined(true)
    {
    }

    void SetUndefined()
    {
        bUndefined = true;
    }

    bool IsUndefined() const
    {
        return bUndefined;
    }

    void SetDefined()
    {
        bUndefined = false;
    }

private:
    bool bUndefined;
};