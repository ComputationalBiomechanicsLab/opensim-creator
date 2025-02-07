#pragma once

#include <liboscar/Concepts/AssociativeContainer.h>

#include <concepts>
#include <ranges>

namespace osc
{
    // Satisfied if `Key` is a valid key for the given associative
    // container `Container`.
    template<typename Key, typename Container>
    concept AssociativeContainerKey = requires(Container& c, Key key) {
        requires AssociativeContainer<Container>;
        { c.find(key) } -> std::same_as<std::ranges::iterator_t<Container>>;
    };
}
