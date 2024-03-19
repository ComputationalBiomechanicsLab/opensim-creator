#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/IVec2OutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/IStringOutputValueExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>

#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>
#include <string>

namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }

namespace osc
{
    // an output extractor that concatenates the outputs from multiple output extractors
    class ConcatenatingOutputExtractor final :
        public IOutputExtractor,
        private IVec2OutputValueExtractor,
        private IStringOutputValueExtractor {
    public:
        ConcatenatingOutputExtractor(OutputExtractor first_, OutputExtractor second_);

    private:
        CStringView implGetName() const override { return m_Label; }
        CStringView implGetDescription() const override { return {}; }
        void implAccept(IOutputValueExtractorVisitor&) const override;
        size_t implGetHash() const override;
        bool implEquals(IOutputExtractor const&) const override;

        void implExtractFloats(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::span<Vec2> overwriteOut
        ) const override;

        std::string implExtractString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const override;

        OutputExtractor m_First;
        OutputExtractor m_Second;
        OutputExtractorDataType m_OutputType;
        std::string m_Label;
    };
}
