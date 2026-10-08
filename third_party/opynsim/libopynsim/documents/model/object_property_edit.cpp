#include "object_property_edit.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>

#include <functional>
#include <string>
#include <utility>

using namespace osc;

namespace
{
    // returns the absolute path to the object if it's a components; otherwise, returns
    // an empty string
    std::string get_abs_path_or_empty_if_not_a_component(const OpenSim::Object& obj)
    {
        if (const auto* c = dynamic_cast<const OpenSim::Component*>(&obj)) {
            return opyn::get_absolute_path_string(*c);
        }
        else {
            return std::string{};
        }
    }
}

osc::ObjectPropertyEdit::ObjectPropertyEdit(
    const OpenSim::AbstractProperty& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    property_name_{prop.getName()},
    updater_{std::move(updater)}
{}
osc::ObjectPropertyEdit::ObjectPropertyEdit(
    const OpenSim::Object& obj,
    const OpenSim::AbstractProperty& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    component_abs_path_{get_abs_path_or_empty_if_not_a_component(obj)},
    property_name_{prop.getName()},
    updater_{std::move(updater)}
{}
const std::string& osc::ObjectPropertyEdit::get_component_abs_path() const
{
    return component_abs_path_;
}

const std::string& osc::ObjectPropertyEdit::get_property_name() const
{
    return property_name_;
}

void osc::ObjectPropertyEdit::apply(OpenSim::AbstractProperty& prop)
{
    updater_(prop);
}
