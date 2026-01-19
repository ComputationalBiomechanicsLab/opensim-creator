#pragma once

#include <libopynsim/Documents/OutputExtractors/IOutputExtractor.h>
#include <libopynsim/Documents/OutputExtractors/OutputValueExtractor.h>
#include <libopynsim/Documents/StateViewWithMetadata.h>

#include <liboscar/utils/c_string_view.h>

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
    // concrete reference-counted value-type wrapper for an `IOutputExtractor`.
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class OutputExtractor final {
    public:
        template<typename ConcreteIOutputExtractor>
        explicit OutputExtractor(ConcreteIOutputExtractor&& output) :
            m_Output{std::make_shared<ConcreteIOutputExtractor>(std::forward<ConcreteIOutputExtractor>(output))}
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
            std::convertible_to<std::ranges::range_const_reference_t<R>, const StateViewWithMetadata&>
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
            std::convertible_to<std::ranges::range_const_reference_t<R>, const StateViewWithMetadata&>
        )
        std::vector<T> slurpValues(const OpenSim::Component& component, const R& states) const
        {
            return m_Output->slurpValues<T>(component, states);
        }

        size_t getHash() const { return m_Output->getHash(); }

        bool equals(const IOutputExtractor& other) const { return m_Output->equals(other); }
        operator const IOutputExtractor& () const { return *m_Output; }
        const IOutputExtractor& getInner() const { return *m_Output; }

        friend bool operator==(const OutputExtractor& lhs, const OutputExtractor& rhs)
        {
            return *lhs.m_Output == *rhs.m_Output;
        }
    private:
        friend std::string to_string(const OutputExtractor&);
        friend struct std::hash<OutputExtractor>;

        std::shared_ptr<const IOutputExtractor> m_Output;
    };

    template<std::derived_from<IOutputExtractor> ConcreteOutputExtractor, typename... Args>
    requires std::constructible_from<ConcreteOutputExtractor, Args&&...>
    OutputExtractor make_output_extractor(Args&&... args)
    {
        return OutputExtractor{ConcreteOutputExtractor{std::forward<Args>(args)...}};
    }

    std::ostream& operator<<(std::ostream&, const OutputExtractor&);
    std::string to_string(const OutputExtractor&);
}

template<>
struct std::hash<osc::OutputExtractor> final {
    size_t operator()(const osc::OutputExtractor& o) const
    {
        return o.m_Output->getHash();
    }
};
