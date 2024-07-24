#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const osc::Vec4& v) { x = v.x; y = v.y; z = v.z; w = v.w; }  \
        operator osc::Vec4() const { return osc::Vec4(x, y, z, w); }        \
        ImVec4(const osc::Color& v) { x = v.r; y = v.g; z = v.b; w = v.a; } \
        operator osc::Color() const { return osc::Color{x, y, z, w};        }

#define IM_VEC2_CLASS_EXTRA                                                 \
         ImVec2(const osc::Vec2& f) { x = f.x; y = f.y; }                   \
         operator osc::Vec2() const { return osc::Vec2(x,y); }
