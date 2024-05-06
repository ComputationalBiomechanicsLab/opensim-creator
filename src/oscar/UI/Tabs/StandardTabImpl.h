#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <string>
#include <string_view>

namespace osc
{
    class StandardTabImpl : public ITab {
    protected:
        explicit StandardTabImpl(std::string_view tab_name) :
            name_{std::string{tab_name}}
        {}

    private:
        UID impl_get_id() const final
        {
            return id_;
        }

        CStringView impl_get_name() const final
        {
            return name_;
        }

        UID id_;
        std::string name_;
    };
}
