#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Utils/ParamBlock.h>

#include <vector>

namespace osc
{
    // an environment that can be optionally associated with, multiple
    // `IModelStatePair`s (e.g. they all operate "in the same environment")
    class Environment final {
    public:
        Environment();

        const ParamBlock& getSimulationParams() const { return m_ParamBlock; }
        ParamBlock& updSimulationParams() { return m_ParamBlock; }

        int getNumUserOutputExtractors() const { return static_cast<int>(m_OutputExtractors.size()); }
        const OutputExtractor& getUserOutputExtractor(int index) const;
        void addUserOutputExtractor(const OutputExtractor& extractor);
        void removeUserOutputExtractor(int index);
        bool hasUserOutputExtractor(const OutputExtractor& extractor) const;
        bool removeUserOutputExtractor(const OutputExtractor& extractor);
        bool overwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer);

        std::vector<OutputExtractor> getAllUserOutputExtractors() const;
    private:
        // simulation params: dictates how the next simulation shall be ran
        ParamBlock m_ParamBlock;

        // user-initiated output extractors
        //
        // simulators should try to hook into these, if the component exists
        std::vector<OutputExtractor> m_OutputExtractors;
    };
}
