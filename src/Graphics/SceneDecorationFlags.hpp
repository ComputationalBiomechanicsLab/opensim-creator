#pragma once

namespace osc
{
    using SceneDecorationFlags = int;
    enum SceneDecorationFlags_ {
        SceneDecorationFlags_None = 0,
        SceneDecorationFlags_IsSelected = 1<<0,
        SceneDecorationFlags_IsChildOfSelected = 1<<1,
        SceneDecorationFlags_IsHovered = 1<<2,
        SceneDecorationFlags_IsChildOfHovered = 1<<3,
        SceneDecorationFlags_IsIsolated = 1<<4,
        SceneDecorationFlags_IsChildOfIsolated = 1<<5,
    };
}