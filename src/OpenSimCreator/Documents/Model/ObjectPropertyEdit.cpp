#include "ObjectPropertyEdit.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

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
    std::string GetAbsPathOrEmptyIfNotAComponent(const OpenSim::Object& obj)
    {
        if (auto const* c = dynamic_cast<OpenSim::Component const*>(&obj)) {
            return GetAbsolutePathString(*c);
        }
        else {
            return std::string{};
        }
    }
}

osc::ObjectPropertyEdit::ObjectPropertyEdit(
    const OpenSim::AbstractProperty& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    m_PropertyName{prop.getName()},
    m_Updater{std::move(updater)}
{}
osc::ObjectPropertyEdit::ObjectPropertyEdit(
    const OpenSim::Object& obj,
    const OpenSim::AbstractProperty& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    m_ComponentAbsPath{GetAbsPathOrEmptyIfNotAComponent(obj)},
    m_PropertyName{prop.getName()},
    m_Updater{std::move(updater)}
{}
const std::string& osc::ObjectPropertyEdit::getComponentAbsPath() const
{
    return m_ComponentAbsPath;
}

const std::string& osc::ObjectPropertyEdit::getPropertyName() const
{
    return m_PropertyName;
}

void osc::ObjectPropertyEdit::apply(OpenSim::AbstractProperty& prop)
{
    m_Updater(prop);
}
