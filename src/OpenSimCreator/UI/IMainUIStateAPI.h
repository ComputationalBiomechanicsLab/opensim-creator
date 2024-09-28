#pragma once

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/UID.h>

#include <concepts>
#include <memory>
#include <utility>

namespace osc { class ParamBlock; }
namespace osc { class OutputExtractor; }

namespace osc
{
    // API access to shared state between main UI tabs
    //
    // this is how individual UI tabs inter-communicate (e.g. by sharing data, closing other tabs, etc.)
    class IMainUIStateAPI {
    protected:
        IMainUIStateAPI() = default;
        IMainUIStateAPI(const IMainUIStateAPI&) = default;
        IMainUIStateAPI(IMainUIStateAPI&&) noexcept = default;
        IMainUIStateAPI& operator=(const IMainUIStateAPI&) = default;
        IMainUIStateAPI& operator=(IMainUIStateAPI&&) noexcept = default;
    public:
        virtual ~IMainUIStateAPI() noexcept = default;

        template<std::derived_from<Tab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        UID add_tab(Args&&... args)
        {
            return add_tab(std::make_unique<T>(std::forward<Args>(args)...));
        }

        UID add_tab(std::unique_ptr<Tab> tab) { return impl_add_tab(std::move(tab)); }
        void select_tab(UID tab_id) { impl_select_tab(tab_id); }
        void close_tab(UID tab_id) { impl_close_tab(tab_id); }
        void reset_imgui() { impl_reset_imgui(); }

        template<std::derived_from<Tab> T, typename... Args>
        requires std::constructible_from<T, Args&&...>
        void add_and_select_tab(Args&&... args)
        {
            const UID tab_id = add_tab<T>(std::forward<Args>(args)...);
            select_tab(tab_id);
        }

        const ParamBlock& getSimulationParams() const
        {
            return implGetSimulationParams();
        }
        ParamBlock& updSimulationParams()
        {
            return implUpdSimulationParams();
        }

        int getNumUserOutputExtractors() const
        {
            return implGetNumUserOutputExtractors();
        }
        const OutputExtractor& getUserOutputExtractor(int index) const
        {
            return implGetUserOutputExtractor(index);
        }
        void addUserOutputExtractor(const OutputExtractor& extractor)
        {
            return implAddUserOutputExtractor(extractor);
        }
        void removeUserOutputExtractor(int index)
        {
            implRemoveUserOutputExtractor(index);
        }
        bool hasUserOutputExtractor(const OutputExtractor& extractor) const
        {
            return implHasUserOutputExtractor(extractor);
        }
        bool removeUserOutputExtractor(const OutputExtractor& extractor)
        {
            return implRemoveUserOutputExtractor(extractor);
        }
        bool overwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer)
        {
            return implOverwriteOrAddNewUserOutputExtractor(old, newer);
        }

    private:
        virtual UID impl_add_tab(std::unique_ptr<Tab>) = 0;
        virtual void impl_select_tab(UID) = 0;
        virtual void impl_close_tab(UID) = 0;
        virtual void impl_reset_imgui() {}

        virtual const ParamBlock& implGetSimulationParams() const = 0;
        virtual ParamBlock& implUpdSimulationParams() = 0;

        virtual int implGetNumUserOutputExtractors() const = 0;
        virtual const OutputExtractor& implGetUserOutputExtractor(int) const = 0;
        virtual void implAddUserOutputExtractor(const OutputExtractor&) = 0;
        virtual void implRemoveUserOutputExtractor(int) = 0;
        virtual bool implHasUserOutputExtractor(const OutputExtractor&) const = 0;
        virtual bool implRemoveUserOutputExtractor(const OutputExtractor&) = 0;
        virtual bool implOverwriteOrAddNewUserOutputExtractor(const OutputExtractor&, const OutputExtractor&) = 0;
    };
}
