#pragma once

#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace osc { class SimulationReport; }
namespace SimTK { class MultibodySystem; }

namespace osc
{
    // an output extractor that uses a free-function to extract a single value from
    // a SimTK::MultiBodySystem
    //
    // handy for extracting simulation stats (e.g. num steps taken etc.)
    class MultiBodySystemOutputExtractor final : public VirtualOutputExtractor {
    public:
        using ExtractorFn = float (*)(SimTK::MultibodySystem const&);

        MultiBodySystemOutputExtractor(std::string_view name,
                                       std::string_view description,
                                       ExtractorFn extractor);

        std::string const& getName() const override;
        std::string const& getDescription() const override;

        OutputType getOutputType() const override;
        float getValueFloat(OpenSim::Component const&,
                            SimulationReport const&) const override;
        void getValuesFloat(OpenSim::Component const&,
                            nonstd::span<SimulationReport const>,
                            nonstd::span<float> overwriteOut) const override;
        std::string getValueString(OpenSim::Component const&,
                                   SimulationReport const&) const override;

        UID getAuxiliaryDataID() const;
        ExtractorFn getExtractorFunction() const;

        std::size_t getHash() const override;
        bool equals(VirtualOutputExtractor const&) const override;

    private:
        UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumMultiBodySystemOutputExtractors();
    MultiBodySystemOutputExtractor const& GetMultiBodySystemOutputExtractor(int idx);
    OutputExtractor GetMultiBodySystemOutputExtractorDynamic(int idx);
}
