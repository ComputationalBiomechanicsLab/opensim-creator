#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <string>

namespace osc
{
    class StandardTabImpl : public ITab {
    protected:
        explicit StandardTabImpl(CStringView tabName) :
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
