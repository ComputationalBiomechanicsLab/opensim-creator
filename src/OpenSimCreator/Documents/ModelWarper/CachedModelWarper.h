#pragma once

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>

#include <memory>

namespace OpenSim { class Model; }
namespace osc::mow { class ModelWarpDocument; }

namespace osc::mow
{
    // a class that can warp a `ModelWarpDocument` into a new (warped) model-state
    // pair, that also tries to cache any intermediate datastructures to accelerate
    // subsequent warps
    class CachedModelWarper final {
    public:
        CachedModelWarper();
        CachedModelWarper(CachedModelWarper const&) = delete;
        CachedModelWarper(CachedModelWarper&&) noexcept;
        CachedModelWarper& operator=(CachedModelWarper const&) = delete;
        CachedModelWarper& operator=(CachedModelWarper&&) noexcept;
        ~CachedModelWarper() noexcept;

        std::shared_ptr<IConstModelStatePair const> warp(ModelWarpDocument const&);
    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
