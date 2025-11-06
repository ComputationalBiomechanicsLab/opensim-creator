#pragma once

// `std::mdspan` is not available on almost all target platforms, so
// this shim is used for all of them.

#define MDSPAN_IMPL_STANDARD_NAMESPACE opyn::cpp23
#define MDSPAN_IMPL_PROPOSED_NAMESPACE proposed
#define MDSPAN_USE_BRACKET_OPERATOR 1
#include <mdspan/mdspan.hpp>
