#pragma once

#include "src/Utils/UID.hpp"

#include <memory>
#include <utility>

namespace osc
{
    class Tab;

    class TabHost {
    public:
        virtual ~TabHost() noexcept = default;

        template<typename T, typename... Args>
        UID addTab(Args... args)
        {
            return addTab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        UID addTab(std::unique_ptr<Tab> tab);
        void selectTab(UID);
        void closeTab(UID);

    private:
        virtual UID implAddTab(std::unique_ptr<Tab>) = 0;
        virtual void implSelectTab(UID) = 0;
        virtual void implCloseTab(UID) = 0;
    };
}