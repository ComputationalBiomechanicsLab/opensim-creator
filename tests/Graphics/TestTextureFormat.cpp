#include "src/Graphics/TextureFormat.hpp"

#include <cstdint>
#include <type_traits>

static_assert(std::is_same_v<std::underlying_type_t<osc::TextureFormat>, int32_t>);
