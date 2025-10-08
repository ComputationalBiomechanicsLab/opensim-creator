#include "ComponentRegistryEntryBase.h"

#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <string_view>
#include <utility>

osc::ComponentRegistryEntryBase::ComponentRegistryEntryBase(
    std::string_view name_,
    std::string_view description_,
    std::shared_ptr<const OpenSim::Component> prototype_) :

    m_Name{name_},
    m_Description{description_},
    m_Prototype{std::move(prototype_)}
{}

std::unique_ptr<OpenSim::Component> osc::ComponentRegistryEntryBase::instantiate() const
{
    return Clone(*m_Prototype);
}
