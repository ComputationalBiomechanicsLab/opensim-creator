#include "ShapeFittingTab.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Ellipsoid.hpp>
#include <oscar/Maths/Plane.hpp>
#include <oscar/Maths/Sphere.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UndoRedo.hpp>
#include <oscar/Utils/VariantHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <optional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "OpenSim/ShapeFitting";
}

class osc::ShapeFittingTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
    }

    void implOnUnmount() final
    {
    }

    bool implOnEvent(SDL_Event const&) final
    {
        return false;
    }

    void implOnTick() final
    {
    }

    void implOnDrawMainMenu() final
    {
    }

    void implOnDraw() final
    {
    }
};


// public API

osc::CStringView osc::ShapeFittingTab::id() noexcept
{
    return c_TabStringID;
}

osc::ShapeFittingTab::ShapeFittingTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ShapeFittingTab::ShapeFittingTab(ShapeFittingTab&&) noexcept = default;
osc::ShapeFittingTab& osc::ShapeFittingTab::operator=(ShapeFittingTab&&) noexcept = default;
osc::ShapeFittingTab::~ShapeFittingTab() noexcept = default;

osc::UID osc::ShapeFittingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ShapeFittingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ShapeFittingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ShapeFittingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ShapeFittingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ShapeFittingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ShapeFittingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ShapeFittingTab::implOnDraw()
{
    m_Impl->onDraw();
}
