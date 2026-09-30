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
namespace SimTK { class MultibodySystem; }

namespace opyn
{
    // an output extractor that uses a free-function to extract a single value from
    // a SimTK::MultiBodySystem
    //
    // handy for extracting simulation stats (e.g. num steps taken etc.)
    class MultiBodySystemOutputExtractor final : public OutputExtractor {
    public:
        using ExtractorFn = float (*)(const SimTK::MultibodySystem&);

        MultiBodySystemOutputExtractor(
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
        OutputExtractorDataType impl_output_type() const final { return opyn::OutputExtractorDataType::Float; }
        OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const final;
        size_t impl_hash() const final;
        bool impl_equals(const OutputExtractor&) const final;

        osc::UID auxiliary_data_id_;
        std::string name_;
        std::string description_;
        ExtractorFn extractor_;
    };

    int num_multi_body_system_output_extractors();
    const MultiBodySystemOutputExtractor& multi_body_system_output_extractor(int idx);
    SharedOutputExtractor multi_body_system_output_extractor_dynamic(int idx);
}
