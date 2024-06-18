#pragma once

#include <OpenSimCreator/Documents/OutputExtractors/ComponentOutputSubfield.h>
#include <OpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputValueExtractor.h>

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ClonePtr.h>

#include <cstddef>

namespace OpenSim { class AbstractOutput; }
namespace OpenSim { class ComponentPath; }

namespace osc
{
    // an output extractor that uses the `OpenSim::AbstractOutput` API to extract a value
    // from a component
    class ComponentOutputExtractor final : public IOutputExtractor {
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
        CStringView implGetName() const final;
        CStringView implGetDescription() const final;
        OutputExtractorDataType implGetOutputType() const final;
        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final;
        size_t implGetHash() const final;
        bool implEquals(const IOutputExtractor&) const final;

        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}
