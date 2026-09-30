#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <string>
#include <string_view>

namespace opyn
{
    class ComponentRegistryEntryBase {
    public:
        ComponentRegistryEntryBase(
            std::string_view name,
            std::string_view description,
            std::shared_ptr<const OpenSim::Component>
        );

        osc::CStringView name() const { return name_; }
        osc::CStringView description() const { return description_; }
        const OpenSim::Component& prototype() const { return *prototype_; }
        std::unique_ptr<OpenSim::Component> instantiate() const;

    private:
        std::string name_;
        std::string description_;
        std::shared_ptr<const OpenSim::Component> prototype_;
    };
}
