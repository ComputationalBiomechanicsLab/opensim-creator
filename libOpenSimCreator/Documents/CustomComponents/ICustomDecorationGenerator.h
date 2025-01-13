#pragma once

#include <functional>

namespace osc { struct SceneDecoration; }
namespace SimTK { class State; }

namespace osc
{
    // an interface for an object that generates decorations in an OSC-specific custom way
    //
    // i.e. if an `OpenSim::Component` implements this, it should take precedence over `Component::generateDecorations`
    class ICustomDecorationGenerator {
    protected:
        ICustomDecorationGenerator() = default;
        ICustomDecorationGenerator(const ICustomDecorationGenerator&) = default;
        ICustomDecorationGenerator(ICustomDecorationGenerator&&) noexcept = default;
        ICustomDecorationGenerator& operator=(const ICustomDecorationGenerator&) = default;
        ICustomDecorationGenerator& operator=(ICustomDecorationGenerator&&) noexcept = default;
    public:
        virtual ~ICustomDecorationGenerator() noexcept = default;

        void generateCustomDecorations(
            const SimTK::State& state,
            const std::function<void(SceneDecoration&&)>& callback) const
        {
            implGenerateCustomDecorations(state, callback);
        }
    private:
        virtual void implGenerateCustomDecorations(
            const SimTK::State&,
            const std::function<void(SceneDecoration&&)>&
        ) const = 0;
    };
}
