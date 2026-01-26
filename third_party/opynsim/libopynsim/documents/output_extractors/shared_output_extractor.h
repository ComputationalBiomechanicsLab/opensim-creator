#pragma once

#include <libopynsim/documents/output_extractors/output_extractor.h>
#include <libopynsim/documents/output_extractors/output_value_extractor.h>
#include <libopynsim/documents/state_view_with_metadata.h>

#include <liboscar/utilities/c_string_view.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

namespace osc
{
    // concrete reference-counted value-type wrapper for an `OutputExtractor`.
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class SharedOutputExtractor final {
    public:
        template<typename ConcreteOutputExtractor>
        explicit SharedOutputExtractor(ConcreteOutputExtractor&& output) :
            m_Output{std::make_shared<ConcreteOutputExtractor>(std::forward<ConcreteOutputExtractor>(output))}
        {}

        CStringView getName() const { return m_Output->getName(); }
        CStringView getDescription() const { return m_Output->getDescription(); }
        OutputExtractorDataType getOutputType() const { return m_Output->getOutputType(); }

        OutputValueExtractor getOutputValueExtractor(const OpenSim::Component& component) const
        {
            return m_Output->getOutputValueExtractor(component);
        }

        template<typename T>
        requires std::constructible_from<T, Variant&&>
        T getValue(const OpenSim::Component& component, const StateViewWithMetadata& state) const
        {
            return m_Output->getValue<T>(component, state);
        }

        template<typename T, std::ranges::forward_range R>
        requires (
            std::constructible_from<T, Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        void getValues(
            const OpenSim::Component& component,
            const R& states,
            const std::function<void(T)>& consumer) const
        {
            return m_Output->getValues<T>(component, states, consumer);
        }

        template<typename T, std::ranges::forward_range R>
        requires (
            std::constructible_from<T, Variant&&> and
            std::convertible_to<std::ranges::range_value_t<R>, const StateViewWithMetadata&>
        )
        std::vector<T> slurpValues(const OpenSim::Component& component, const R& states) const
        {
            return m_Output->slurpValues<T>(component, states);
        }

        size_t getHash() const { return m_Output->getHash(); }

        bool equals(const OutputExtractor& other) const { return m_Output->equals(other); }
        operator const OutputExtractor& () const { return *m_Output; }
        const OutputExtractor& getInner() const { return *m_Output; }

        friend bool operator==(const SharedOutputExtractor& lhs, const SharedOutputExtractor& rhs)
        {
            return *lhs.m_Output == *rhs.m_Output;
        }
    private:
        friend std::string to_string(const SharedOutputExtractor&);
        friend struct std::hash<SharedOutputExtractor>;

        std::shared_ptr<const OutputExtractor> m_Output;
    };

    template<std::derived_from<OutputExtractor> ConcreteOutputExtractor, typename... Args>
    requires std::constructible_from<ConcreteOutputExtractor, Args&&...>
    SharedOutputExtractor make_output_extractor(Args&&... args)
    {
        return SharedOutputExtractor{ConcreteOutputExtractor{std::forward<Args>(args)...}};
    }

    std::ostream& operator<<(std::ostream&, const SharedOutputExtractor&);
    std::string to_string(const SharedOutputExtractor&);
}

template<>
struct std::hash<osc::SharedOutputExtractor> final {
    size_t operator()(const osc::SharedOutputExtractor& o) const
    {
        return o.m_Output->getHash();
    }
};
