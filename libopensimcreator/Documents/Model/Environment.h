#pragma once

#include <libopensimcreator/Documents/ParamBlock.h>

#include <libopynsim/Documents/output_extractors/shared_output_extractor.h>

#include <vector>

namespace osc
{
    // an environment that can be optionally associated with, multiple
    // `ModelStatePair`s (e.g. they all operate "in the same environment")
    class Environment final {
    public:
        Environment();

        const ParamBlock& getSimulationParams() const { return m_ParamBlock; }
        ParamBlock& updSimulationParams() { return m_ParamBlock; }

        int getNumUserOutputExtractors() const { return static_cast<int>(m_OutputExtractors.size()); }
        const SharedOutputExtractor& getUserOutputExtractor(int index) const;
        void addUserOutputExtractor(const SharedOutputExtractor& extractor);
        void removeUserOutputExtractor(int index);
        bool hasUserOutputExtractor(const SharedOutputExtractor& extractor) const;
        bool removeUserOutputExtractor(const SharedOutputExtractor& extractor);
        bool overwriteOrAddNewUserOutputExtractor(const SharedOutputExtractor& old, const SharedOutputExtractor& newer);

        std::vector<SharedOutputExtractor> getAllUserOutputExtractors() const;
    private:
        // simulation params: dictates how the next simulation shall be ran
        ParamBlock m_ParamBlock;

        // user-initiated output extractors
        //
        // simulators should try to hook into these, if the component exists
        std::vector<SharedOutputExtractor> m_OutputExtractors;
    };
}
