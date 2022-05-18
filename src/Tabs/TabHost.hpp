#pragma once

#include "src/Utils/UID.hpp"

#include <memory>

namespace osc
{
    class Tab;

    class TabHost {
    public:
        virtual ~TabHost() noexcept = default;

        void addTab(std::unique_ptr<Tab> tab);
        void selectTab(UID);
        void closeTab(UID);

    private:
        virtual void implAddTab(std::unique_ptr<Tab>) = 0;
        virtual void implSelectTab(UID) = 0;
        virtual void implCloseTab(UID) = 0;
    };
}