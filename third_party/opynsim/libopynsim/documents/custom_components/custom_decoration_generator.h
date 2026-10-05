#pragma once

#include <functional>

namespace osc { struct SceneDecoration; }
namespace SimTK { class State; }

namespace opyn
{
    // an interface for an object that generates decorations in an OSC-specific custom way
    //
    // i.e. if an `OpenSim::Component` implements this, it should take precedence over `Component::generateDecorations`
    class CustomDecorationGenerator {
    protected:
        CustomDecorationGenerator() = default;
        CustomDecorationGenerator(const CustomDecorationGenerator&) = default;
        CustomDecorationGenerator(CustomDecorationGenerator&&) noexcept = default;
        CustomDecorationGenerator& operator=(const CustomDecorationGenerator&) = default;
        CustomDecorationGenerator& operator=(CustomDecorationGenerator&&) noexcept = default;
    public:
        virtual ~CustomDecorationGenerator() noexcept = default;

        void generate_custom_decorations(
            const SimTK::State& state,
            const std::function<void(osc::SceneDecoration&&)>& callback) const
        {
            impl_generate_custom_decorations(state, callback);
        }
    private:
        virtual void impl_generate_custom_decorations(
            const SimTK::State&,
            const std::function<void(osc::SceneDecoration&&)>&
        ) const = 0;
    };
}
