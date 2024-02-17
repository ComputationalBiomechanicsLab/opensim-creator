#pragma once

#include <cstddef>

namespace osc
{
    // top-level template that's specialized by `Vec2`, `Vec3`, and `Vec4`
    //
    // history (applies to `Vec`, `Mat`, and `Qua`)
    // ============================================
    //
    // most of the maths implemenatation was initially copied from `glm`,
    // which is a much more extensive (and heavily tested!) library. If
    // you're reading this because you're trying to decide on a maths library
    // to use, you should probably use `glm`. `oscar`'s maths library is
    // specifically  designed to aggressively interoperate with `oscar`
    // and the C++ standard library, rather than be a comprehensive maths
    // library
    //
    // why not keep using glm?
    // =======================
    //
    // earlier versions of `oscar` directly used `glm` all over the
    // codebase but it became quite ugly - plus some additional helper
    // functions were desired. So, for a while, `oscar`'s math types were
    // just aliases to make things less ugly:
    //
    //     using Vec3 = glm::vec3;
    //
    // later more functions (+templates) were added. This started to
    // get uglier, because the templated alias had additional glm-specific
    // things like `Qualifier`, etc. (not too bad to handle) but, it being
    // an alias, also created scenarios in which ADL started to interact
    // with the fact that some helper methods were defined in a different
    // namespace (osc:: vs glm::) from the type, etc.
    //
    // the longer-term picture is that `oscar` was aggressively transitioned
    // to C++20, which has better support for stuff like concepts, but
    // which weren't available in glm. Looking further, the vision on the
    // horizon was to adopt C++23, which was set to include `constexpr` maths
    // functions (sqrt, sin, etc.), followed by adopting C++26, which
    // was set to include `linalg` (libBLAS/libLAPACK) - these are things that
    // `oscar` would like to adapt as-soon-as-possible without any library
    // lag or weird translation layers (`oscar`'s philosopy is "do what you
    // can with the standard library")
    //
    // ... and there's a dream of shipping `oscar` as a library, but I didn't
    //     want the dependency hell of shipping a library that exposes `glm` to
    //     downstream projects that are likely to, themselves, have a different
    //     version of glm... ;)
    template<size_t L, typename T>
    struct Vec;
}
