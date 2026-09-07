#pragma once

#include <nanobind/nanobind.h>

#include <memory>

namespace opyn
{
    /// Wrap a unique pointer to `T` into a capsule.
    template<typename T>
    nanobind::capsule to_capsule(std::unique_ptr<T> p)
    {
        return nanobind::capsule{
            p.release(),
            [](void* ptr) noexcept { delete static_cast<T*>(ptr); }  // NOLINT(cppcoreguidelines-owning-memory)
        };
    }

    /// Wrap a unique pointer to an array of `T` into a capsule.
    template<typename T>
    nanobind::capsule to_capsule(std::unique_ptr<T[]> p)
    {
        return nanobind::capsule{
            p.release(),
            [](void* ptr) noexcept { delete[] static_cast<T*>(ptr); }  // NOLINT(cppcoreguidelines-owning-memory)
        };
    }
}
