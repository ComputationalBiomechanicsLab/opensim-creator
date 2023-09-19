#include "ComponentRegistryEntryBase.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

osc::ComponentRegistryEntryBase::ComponentRegistryEntryBase(
    std::string_view name_,
    std::string_view description_,
    std::shared_ptr<OpenSim::Component const> prototype_) :

    m_Name{name_},
    m_Description{description_},
    m_Prototype{std::move(prototype_)}
{
}

std::unique_ptr<OpenSim::Component> osc::ComponentRegistryEntryBase::instantiate() const
{
    return Clone(*m_Prototype);
}
