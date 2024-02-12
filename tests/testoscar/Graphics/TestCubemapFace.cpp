#include <oscar/Graphics/CubemapFace.h>

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/EnumHelpers.h>

namespace cpp23 = osc::cpp23;
using namespace osc;

static_assert(NumOptions<CubemapFace>() == 6);

// the sequence of faces is important, because OpenGL defines a texture target
// macro (e.g. GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
// that exactly follows this sequence, and code in the backend might rely on that
//
// (e.g. because there's code along the lines of `GL_TEXTURE_CUBE_MAP_POSITIVE_X+i`
static_assert(cpp23::to_underlying(CubemapFace::PositiveX) == 0);
static_assert(cpp23::to_underlying(CubemapFace::NegativeX) == 1);
static_assert(cpp23::to_underlying(CubemapFace::PositiveY) == 2);
static_assert(cpp23::to_underlying(CubemapFace::NegativeY) == 3);
static_assert(cpp23::to_underlying(CubemapFace::PositiveZ) == 4);
static_assert(cpp23::to_underlying(CubemapFace::NegativeZ) == 5);
