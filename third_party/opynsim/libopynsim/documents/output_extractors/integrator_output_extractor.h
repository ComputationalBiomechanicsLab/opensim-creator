#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace SimTK { class Integrator; }

namespace opyn
{
    // an output extractor that extracts integrator metadata (e.g. predicted step size)
    class IntegratorOutputExtractor final : public OutputExtractor {
    public:
        using ExtractorFn = float (*)(const SimTK::Integrator&);

        IntegratorOutputExtractor(
            std::string_view name,
            std::string_view description,
            ExtractorFn extractor) :

            name_{name},
            description_{description},
            extractor_{extractor}
        {}

        osc::UID auxiliary_data_id() const { return auxiliary_data_id_; }
        ExtractorFn extractor_function() const { return extractor_; }

    private:
        osc::CStringView impl_name() const final { return name_; }
        osc::CStringView impl_description() const final { return description_; }
        OutputExtractorDataType impl_output_type() const override { return OutputExtractorDataType::Float; }
        OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const final;
        size_t impl_hash() const final;
        bool impl_equals(const OutputExtractor&) const final;

        osc::UID auxiliary_data_id_;
        std::string name_;
        std::string description_;
        ExtractorFn extractor_;
    };

    int num_integrator_output_extractors();
    const IntegratorOutputExtractor& integrator_output_extractor(int idx);
    SharedOutputExtractor integrator_output_extractor_dynamic(int idx);
}
