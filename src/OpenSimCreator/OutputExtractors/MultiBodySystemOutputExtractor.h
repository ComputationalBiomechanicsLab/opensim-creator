#pragma once

#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <span>
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
    class MultiBodySystemOutputExtractor final : public IOutputExtractor {
    public:
        using ExtractorFn = float (*)(SimTK::MultibodySystem const&);

        MultiBodySystemOutputExtractor(
            std::string_view name,
            std::string_view description,
            ExtractorFn extractor
        );

        CStringView getName() const final;
        CStringView getDescription() const final;

        OutputType getOutputType() const final;

        float getValueFloat(
            OpenSim::Component const&,
            SimulationReport const&
        ) const final;

        void getValuesFloat(
            OpenSim::Component const&,
            std::span<SimulationReport const>,
            std::span<float> overwriteOut
        ) const final;

        std::string getValueString(
            OpenSim::Component const&,
            SimulationReport const&
        ) const final;

        UID getAuxiliaryDataID() const;
        ExtractorFn getExtractorFunction() const;

        size_t getHash() const final;
        bool equals(IOutputExtractor const&) const final;

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
