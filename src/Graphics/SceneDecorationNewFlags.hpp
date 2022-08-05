#pragma once

namespace osc
{
    using SceneDecorationNewFlags = int;
    enum SceneDecorationNewFlags_ {
        SceneDecorationNewFlags_None = 0,
        SceneDecorationNewFlags_IsSelected = 1<<0,
        SceneDecorationNewFlags_IsChildOfSelected = 1<<1,
        SceneDecorationNewFlags_IsHovered = 1<<2,
        SceneDecorationNewFlags_IsChildOfHovered = 1<<3,
        SceneDecorationNewFlags_IsIsolated = 1<<4,
        SceneDecorationNewFlags_IsChildOfIsolated = 1<<5,
    };
}