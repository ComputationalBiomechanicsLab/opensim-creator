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

        const OpenSim::ComponentPath& getComponentAbsPath() const;

    private:
        osc::CStringView implGetName() const final;
        osc::CStringView implGetDescription() const final;
        OutputExtractorDataType implGetOutputType() const final;
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final;
        size_t implGetHash() const final;
        bool implEquals(const OutputExtractor&) const final;

        class Impl;
        osc::ClonePtr<Impl> m_Impl;
    };
}
