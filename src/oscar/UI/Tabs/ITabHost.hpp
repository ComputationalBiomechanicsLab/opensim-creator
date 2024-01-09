#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>
#include <utility>

namespace osc
{
    // a virtual interface to something that can host multiple UI tabs
    class ITabHost {
    protected:
        ITabHost() = default;
        ITabHost(ITabHost const&) = default;
        ITabHost(ITabHost&&) noexcept = default;
        ITabHost& operator=(ITabHost const&) = default;
        ITabHost& operator=(ITabHost&&) noexcept = default;
    public:
        virtual ~ITabHost() noexcept = default;

        template<typename T, typename... Args>
        UID addTab(Args&&... args)
        {
            return addTab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        UID addTab(std::unique_ptr<ITab> tab)
        {
            return implAddTab(std::move(tab));
        }

        void selectTab(UID tabID)
        {
            implSelectTab(tabID);
        }

        void closeTab(UID tabID)
        {
            implCloseTab(tabID);
        }

        void resetImgui()
        {
            implResetImgui();
        }

        template<typename T, typename... Args>
        void addAndSelectTab(Args&&... args)
        {
            UID const tabID = addTab<T>(std::forward<Args>(args)...);
            selectTab(tabID);
        }

    private:
        virtual UID implAddTab(std::unique_ptr<ITab>) = 0;
        virtual void implSelectTab(UID) = 0;
        virtual void implCloseTab(UID) = 0;
        virtual void implResetImgui() {}
    };
}
