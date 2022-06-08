#pragma once

#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/Utils/UID.hpp"

#include <optional>
#include <string>
#include <string_view>


namespace OpenSim
{
    class Component;
}

namespace osc
{
    class SimulationReport;
}

namespace SimTK
{
    class Integrator;
}


namespace osc
{
    // an output extractor that extracts integrator metadata (e.g. predicted step size)
    class IntegratorOutputExtractor final : public VirtualOutputExtractor {
    public:
        using ExtractorFn = float (*)(SimTK::Integrator const&);

        IntegratorOutputExtractor(std::string_view name,
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

        std::size_t getHash() const override;
        bool equals(VirtualOutputExtractor const&) const override;

        UID getAuxiliaryDataID() const;
        ExtractorFn getExtractorFunction() const;

    private:
        UID m_AuxiliaryDataID;
        std::string m_Name;
        std::string m_Description;
        ExtractorFn m_Extractor;
    };

    int GetNumIntegratorOutputExtractors();
    IntegratorOutputExtractor const& GetIntegratorOutputExtractor(int idx);
    OutputExtractor GetIntegratorOutputExtractorDynamic(int idx);
}
