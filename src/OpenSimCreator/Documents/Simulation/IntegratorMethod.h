#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <memory>

namespace SimTK { class Integrator; }
namespace SimTK { class System; }

namespace osc
{
    namespace detail
    {
        // internal enum that stores the runtime option
        enum class IntegratorMethodOption {
            OpenSimManagerDefault,
            ExplicitEuler,
            RungeKutta2,
            RungeKutta3,
            RungeKuttaFeldberg,
            RungeKuttaMerson,
            SemiExplicitEuler2,
            Verlet,
            NUM_OPTIONS,
        };
    }

    // integration methods that are supported by the OpenSim backend
    class IntegratorMethod final {
    public:

        // returns an iterable object that can be used to iterate over all available
        // `IntegratorMethod`s
        static constexpr auto all()
        {
            return make_option_iterable<detail::IntegratorMethodOption>([](detail::IntegratorMethodOption opt)
            {
                return IntegratorMethod{opt};
            });
        }

        constexpr IntegratorMethod() = default;

        friend constexpr bool operator==(IntegratorMethod const&, IntegratorMethod const&) = default;

        CStringView label() const;
        std::unique_ptr<SimTK::Integrator> instantiate(SimTK::System const&) const;
    private:
        constexpr IntegratorMethod(detail::IntegratorMethodOption opt) : m_Option{opt} {}

        detail::IntegratorMethodOption m_Option = detail::IntegratorMethodOption::OpenSimManagerDefault;
    };
}

