#include "PropertyDescriptions.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

void osc::doc::PropertyDescriptions::append(PropertyDescription const& desc)
{
    auto const sameName = [&desc](PropertyDescription const& d)
    {
        return desc.getName() == d.getName();
    };

    if (std::any_of(m_Descriptions.begin(), m_Descriptions.end(), sameName))
    {
        std::stringstream ss;
        ss << desc.getName() << ": cannot add this property to the property descriptions list: another property with the same name already exists";
        throw std::runtime_error{std::move(ss).str()};
    }

    m_Descriptions.push_back(desc);
}
