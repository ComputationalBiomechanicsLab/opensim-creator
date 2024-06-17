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
        IMainUIStateAPI(const IMainUIStateAPI&) = default;
        IMainUIStateAPI(IMainUIStateAPI&&) noexcept = default;
        IMainUIStateAPI& operator=(const IMainUIStateAPI&) = default;
        IMainUIStateAPI& operator=(IMainUIStateAPI&&) noexcept = default;
    public:
        virtual ~IMainUIStateAPI() noexcept = default;

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
