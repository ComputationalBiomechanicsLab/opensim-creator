#pragma once

#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>

namespace osc::mow
{
    class WarpDetail final {
    public:
        WarpDetail(std::string name_, std::string value_) :
            m_Name{std::move(name_)},
            m_Value{std::move(value_)}
        {}

        CStringView name() const { return m_Name; }
        CStringView value() const { return m_Value; }
    private:
        std::string m_Name;
        std::string m_Value;
    };
}
