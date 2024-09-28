#pragma once

#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/LifetimedPtr.h>
#include <oscar/Utils/SharedLifetimeBlock.h>

#include <concepts>
#include <optional>

namespace osc
{
    template<typename T>
    using ParentPtr = LifetimedPtr<T>;

    template<typename TDerived, typename TBase>
    std::optional<ParentPtr<TDerived>> dynamic_parent_cast(const ParentPtr<TBase>& p)
    {
        static_assert(std::derived_from<TDerived, TBase>);

        OSC_ASSERT_ALWAYS(not p.expired() && "orphaned child tried to access a dead parent: this is a development error");

        if (auto* downcasted = dynamic_cast<TDerived*>(p.get())) {
            return ParentPtr<TDerived>{p.watcher(), downcasted};
        }
        else {
            return std::nullopt;
        }
    }
}
