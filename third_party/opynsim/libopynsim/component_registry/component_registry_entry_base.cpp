#include "component_registry_entry_base.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <string_view>
#include <utility>

using namespace opyn;

opyn::ComponentRegistryEntryBase::ComponentRegistryEntryBase(
    std::string_view name,
    std::string_view description,
    std::shared_ptr<const OpenSim::Component> prototype) :

    name_{name},
    description_{description},
    prototype_{std::move(prototype)}
{}

std::unique_ptr<OpenSim::Component> opyn::ComponentRegistryEntryBase::instantiate() const
{
    return Clone(*prototype_);
}
