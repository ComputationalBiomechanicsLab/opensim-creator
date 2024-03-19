#pragma once

#include <OpenSimCreator/OutputExtractors/ComponentOutputSubfield.h>
#include <OpenSimCreator/OutputExtractors/IOutputExtractor.h>

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
            OpenSim::AbstractOutput const&,
            ComponentOutputSubfield = ComponentOutputSubfield::None
        );
        ComponentOutputExtractor(ComponentOutputExtractor const&);
        ComponentOutputExtractor(ComponentOutputExtractor&&) noexcept;
        ComponentOutputExtractor& operator=(ComponentOutputExtractor const&);
        ComponentOutputExtractor& operator=(ComponentOutputExtractor&&) noexcept;
        ~ComponentOutputExtractor() noexcept override;

        OpenSim::ComponentPath const& getComponentAbsPath() const;

    private:
        CStringView implGetName() const final;
        CStringView implGetDescription() const final;
        void implAccept(IOutputValueExtractorVisitor&) const final;
        size_t implGetHash() const final;
        bool implEquals(IOutputExtractor const&) const final;

    private:
        class Impl;
        ClonePtr<Impl> m_Impl;
    };
}
