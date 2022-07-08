#pragma once

namespace osc
{
    using ComponentDecorationFlags = int;
    enum ComponentDecorationFlags_ {
        ComponentDecorationFlags_None = 0,
        ComponentDecorationFlags_IsSelected = 1<<0,
        ComponentDecorationFlags_IsChildOfSelected = 1<<1,
        ComponentDecorationFlags_IsHovered = 1<<2,
        ComponentDecorationFlags_IsChildOfHovered = 1<<3,
        ComponentDecorationFlags_IsIsolated = 1<<4,
        ComponentDecorationFlags_IsChildOfIsolated = 1<<5,
    };
}