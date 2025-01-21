#pragma once

#include <libopensimcreator/Documents/Model/IModelStatePair.h>

#include <memory>

namespace OpenSim { class Model; }
namespace osc::mow { class WarpableModel; }

namespace osc::mow
{
    // a class that can warp a `ModelWarpDocument` into a new (warped) model-state
    // pair, that also tries to cache any intermediate datastructures to accelerate
    // subsequent warps
    class CachedModelWarper final {
    public:
        CachedModelWarper();
        CachedModelWarper(const CachedModelWarper&) = delete;
        CachedModelWarper(CachedModelWarper&&) noexcept;
        CachedModelWarper& operator=(const CachedModelWarper&) = delete;
        CachedModelWarper& operator=(CachedModelWarper&&) noexcept;
        ~CachedModelWarper() noexcept;

        std::shared_ptr<IModelStatePair> warp(const WarpableModel&);
    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
