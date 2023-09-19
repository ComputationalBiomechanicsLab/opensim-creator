#pragma once

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <string>

namespace osc
{
    // provides the ID and name parts of a tab implementation
    class StandardTabBase : public Tab {
    protected:
        explicit StandardTabBase(CStringView tabName) :
            m_TabName{std::string{tabName}}
        {
        }

    private:
        UID implGetID() const final
        {
            return m_TabID;
        }

        CStringView implGetName() const final
        {
            return m_TabName;
        }

        UID m_TabID;
        std::string m_TabName;
    };
}
