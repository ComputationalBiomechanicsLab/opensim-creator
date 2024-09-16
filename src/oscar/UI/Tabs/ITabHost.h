#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/UID.h>

#include <concepts>
#include <memory>
#include <utility>

namespace osc
{
    // a virtual interface to something that can host multiple UI tabs
    class ITabHost {
    protected:
        ITabHost() = default;
        ITabHost(const ITabHost&) = default;
        ITabHost(ITabHost&&) noexcept = default;
        ITabHost& operator=(const ITabHost&) = default;
        ITabHost& operator=(ITabHost&&) noexcept = default;
    public:
        virtual ~ITabHost() noexcept = default;

        template<std::derived_from<ITab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        UID add_tab(Args&&... args)
        {
            return add_tab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        UID add_tab(std::unique_ptr<ITab> tab) { return impl_add_tab(std::move(tab)); }
        void select_tab(UID tab_id) { impl_select_tab(tab_id); }
        void close_tab(UID tab_id) { impl_close_tab(tab_id); }
        void reset_imgui() { impl_reset_imgui(); }

        template<std::derived_from<ITab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        void add_and_select_tab(Args&&... args)
        {
            const UID tab_id = add_tab<T>(std::forward<Args>(args)...);
            select_tab(tab_id);
        }

    private:
        virtual UID impl_add_tab(std::unique_ptr<ITab>) = 0;
        virtual void impl_select_tab(UID) = 0;
        virtual void impl_close_tab(UID) = 0;
        virtual void impl_reset_imgui() {}
    };
}
