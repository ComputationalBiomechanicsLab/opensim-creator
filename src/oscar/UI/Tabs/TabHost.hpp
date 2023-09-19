#pragma once

#include <oscar/Utils/UID.hpp>

#include <memory>
#include <utility>

namespace osc { class Tab; }

namespace osc
{
    class TabHost {
    protected:
        TabHost() = default;
        TabHost(TabHost const&) = default;
        TabHost(TabHost&&) noexcept = default;
        TabHost& operator=(TabHost const&) = default;
        TabHost& operator=(TabHost&&) noexcept = default;
    public:
        virtual ~TabHost() noexcept = default;

        template<typename T, typename... Args>
        UID addTab(Args&&... args)
        {
            return addTab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        UID addTab(std::unique_ptr<Tab>);
        void selectTab(UID);
        void closeTab(UID);
        void resetImgui();

        template<typename T, typename... Args>
        void addAndSelectTab(Args&&... args)
        {
            UID const tabID = addTab<T>(std::forward<Args>(args)...);
            selectTab(tabID);
        }

    private:
        virtual UID implAddTab(std::unique_ptr<Tab>) = 0;
        virtual void implSelectTab(UID) = 0;
        virtual void implCloseTab(UID) = 0;
        virtual void implResetImgui() {}
    };
}
