#include "ObjectPropertyEdit.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>

#include <functional>
#include <string>
#include <utility>


namespace
{
    // returns the absolute path to the object if it's a components; otherwise, returns
    // an empty string
    std::string GetAbsPathOrEmptyIfNotAComponent(OpenSim::Object const& obj)
    {
        if (auto c = dynamic_cast<OpenSim::Component const*>(&obj))
        {
            return osc::GetAbsolutePathString(*c);
        }
        else
        {
            return std::string{};
        }
    }
}

osc::ObjectPropertyEdit::ObjectPropertyEdit(
    OpenSim::AbstractProperty const& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    m_ComponentAbsPath{},
    m_PropertyName{prop.getName()},
    m_Updater{std::move(updater)}
{
}

osc::ObjectPropertyEdit::ObjectPropertyEdit(
    OpenSim::Object const& obj,
    OpenSim::AbstractProperty const& prop,
    std::function<void(OpenSim::AbstractProperty&)> updater) :

    m_ComponentAbsPath{GetAbsPathOrEmptyIfNotAComponent(obj)},
    m_PropertyName{prop.getName()},
    m_Updater{std::move(updater)}
{
}

std::string const& osc::ObjectPropertyEdit::getComponentAbsPath() const
{
    return m_ComponentAbsPath;
}

std::string const& osc::ObjectPropertyEdit::getPropertyName() const
{
    return m_PropertyName;
}

void osc::ObjectPropertyEdit::apply(OpenSim::AbstractProperty& prop)
{
    m_Updater(prop);
}