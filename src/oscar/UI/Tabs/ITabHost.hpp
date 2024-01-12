#pragma once

#include <oscar/UI/Tabs/ITab.hpp>
#include <oscar/Utils/Concepts.hpp>
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

        template<DerivedFrom<ITab> T, typename... Args>
        UID addTab(Args&&... args)
            requires ConstructibleFrom<T, Args&&...>
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

        template<DerivedFrom<ITab> T, typename... Args>
        void addAndSelectTab(Args&&... args)
            requires ConstructibleFrom<T, Args&&...>
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
