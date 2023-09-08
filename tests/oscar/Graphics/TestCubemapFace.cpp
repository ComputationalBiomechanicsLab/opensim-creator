#include "oscar/Graphics/CubemapFace.hpp"

#include "oscar/Utils/EnumHelpers.hpp"

#include <cstdint>
#include <type_traits>

static_assert(osc::NumOptions<osc::CubemapFace>() == 6);

// the sequence of faces is important, because OpenGL defines a texture target
// macro (e.g. GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
// that exactly follows this sequence, and code in the backend might rely on that
//
// (e.g. because there's code along the lines of `GL_TEXTURE_CUBE_MAP_POSITIVE_X+i`
static_assert(static_cast<std::underlying_type_t<osc::CubemapFace>>(osc::CubemapFace::PositiveX) == 0);
static_assert(static_cast<std::underlying_type_t<osc::CubemapFace>>(osc::CubemapFace::NegativeX) == 1);
static_assert(static_cast<std::underlying_type_t<osc::CubemapFace>>(osc::CubemapFace::PositiveY) == 2);
static_assert(static_cast<std::underlying_type_t<osc::CubemapFace>>(osc::CubemapFace::NegativeY) == 3);
static_assert(static_cast<std::underlying_type_t<osc::CubemapFace>>(osc::CubemapFace::PositiveZ) == 4);
static_assert(static_cast<std::underlying_type_t<osc::CubemapFace>>(osc::CubemapFace::NegativeZ) == 5);
