#pragma once

#include <libopynsim/documents/output_extractors/component_output_subfield.h>
#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/clone_ptr.h>

#include <cstddef>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class ComponentPath; }

namespace opyn
{
    // an output extractor that uses the `OpenSim::AbstractOutput` API to extract a value
    // from a component
    class ComponentOutputExtractor final : public OutputExtractor {
    public:
        ComponentOutputExtractor(
            const OpenSim::AbstractOutput&,
            ComponentOutputSubfield = ComponentOutputSubfield::None
        );
        ComponentOutputExtractor(const ComponentOutputExtractor&);
        ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept;
        ComponentOutputExtractor& operator=(const ComponentOutputExtractor&);
        ComponentOutputExtractor& operator=(ComponentOutputExtractor&&) noexcept;
        ~ComponentOutputExtractor() noexcept override;

        const OpenSim::ComponentPath& component_abs_path() const;

    private:
        osc::CStringView impl_name() const final;
        osc::CStringView impl_description() const final;
        OutputExtractorDataType impl_output_type() const final;
        OutputValueExtractor impl_output_value_extractor(const OpenSim::Component&) const final;
        size_t impl_hash() const final;
        bool impl_equals(const OutputExtractor&) const final;

        class Impl;
        osc::ClonePtr<Impl> impl_;
    };
}
