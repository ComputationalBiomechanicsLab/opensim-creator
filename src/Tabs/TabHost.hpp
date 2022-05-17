#pragma once

#include <memory>

namespace osc
{
    class Tab;

    class TabHost {
    public:
        virtual ~TabHost() noexcept = default;
        void addTab(std::unique_ptr<Tab> tab);
        void selectTab(Tab* t);
        void closeTab(Tab* t);

    private:
        virtual void implAddTab(std::unique_ptr<Tab> tab) = 0;
        virtual void implSelectTab(Tab*) = 0;
        virtual void implCloseTab(Tab*) = 0;
    };
}