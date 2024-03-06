#pragma once

#include <functional>

namespace osc { struct SceneDecoration; }
namespace SimTK { class State; }

namespace osc
{
    class ICustomDecorationGenerator {
    protected:
        ICustomDecorationGenerator() = default;
        ICustomDecorationGenerator(ICustomDecorationGenerator const&) = default;
        ICustomDecorationGenerator(ICustomDecorationGenerator&&) noexcept = default;
        ICustomDecorationGenerator& operator=(ICustomDecorationGenerator const&) = default;
        ICustomDecorationGenerator& operator=(ICustomDecorationGenerator&&) noexcept = default;
    public:
        virtual ~ICustomDecorationGenerator() noexcept = default;

        void generateCustomDecorations(SimTK::State const& state, std::function<void(SceneDecoration&&)> const& callback) const { implGenerateCustomDecorations(state, callback); }
    private:
        virtual void implGenerateCustomDecorations(SimTK::State const&, std::function<void(SceneDecoration&&)> const&) const = 0;
    };
}
