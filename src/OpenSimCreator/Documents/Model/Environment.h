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
        const ParamBlock& getSimulationParams() const { return m_ParamBlock; }
        ParamBlock& updSimulationParams() { return m_ParamBlock; }

        int getNumUserOutputExtractors() const { return static_cast<int>(m_OutputExtractors.size()); }
        const OutputExtractor& getUserOutputExtractor(int index) const;
        void addUserOutputExtractor(const OutputExtractor& extractor);
        void removeUserOutputExtractor(int index);
        bool hasUserOutputExtractor(const OutputExtractor& extractor) const;
        bool removeUserOutputExtractor(const OutputExtractor& extractor);
        bool overwriteOrAddNewUserOutputExtractor(const OutputExtractor& old, const OutputExtractor& newer);

    private:
        ParamBlock m_ParamBlock;
        std::vector<OutputExtractor> m_OutputExtractors;
    };
}
