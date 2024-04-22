#pragma once

#include <oscar/UI/Tabs/ITabHost.h>

namespace osc { class ParamBlock; }
namespace osc { class OutputExtractor; }

namespace osc
{
    // API access to shared state between main UI tabs
    //
    // this is how individual UI tabs inter-communicate (e.g. by sharing data, closing other tabs, etc.)
    class IMainUIStateAPI : public ITabHost {
    protected:
        IMainUIStateAPI() = default;
        IMainUIStateAPI(IMainUIStateAPI const&) = default;
        IMainUIStateAPI(IMainUIStateAPI&&) noexcept = default;
        IMainUIStateAPI& operator=(IMainUIStateAPI const&) = default;
        IMainUIStateAPI& operator=(IMainUIStateAPI&&) noexcept = default;
    public:
        virtual ~IMainUIStateAPI() noexcept = default;

        ParamBlock const& getSimulationParams() const
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
        OutputExtractor const& getUserOutputExtractor(int index) const
        {
            return implGetUserOutputExtractor(index);
        }
        void addUserOutputExtractor(OutputExtractor const& extractor)
        {
            return implAddUserOutputExtractor(extractor);
        }
        void removeUserOutputExtractor(int index)
        {
            implRemoveUserOutputExtractor(index);
        }
        bool hasUserOutputExtractor(OutputExtractor const& extractor) const
        {
            return implHasUserOutputExtractor(extractor);
        }
        bool removeUserOutputExtractor(OutputExtractor const& extractor)
        {
            return implRemoveUserOutputExtractor(extractor);
        }
        bool overwriteOrAddNewUserOutputExtractor(OutputExtractor const& old, OutputExtractor const& newer)
        {
            return implOverwriteOrAddNewUserOutputExtractor(old, newer);
        }

    private:
        virtual ParamBlock const& implGetSimulationParams() const = 0;
        virtual ParamBlock& implUpdSimulationParams() = 0;

        virtual int implGetNumUserOutputExtractors() const = 0;
        virtual OutputExtractor const& implGetUserOutputExtractor(int) const = 0;
        virtual void implAddUserOutputExtractor(OutputExtractor const&) = 0;
        virtual void implRemoveUserOutputExtractor(int) = 0;
        virtual bool implHasUserOutputExtractor(OutputExtractor const&) const = 0;
        virtual bool implRemoveUserOutputExtractor(OutputExtractor const&) = 0;
        virtual bool implOverwriteOrAddNewUserOutputExtractor(OutputExtractor const&, OutputExtractor const&) = 0;
    };
}
