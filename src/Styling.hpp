#pragma once

// expose macros for in-app icons
#include "third_party/IconsFontAwesome5.h"

// theme color for positive things (saving, opening, adding)
#define OSC_POSITIVE_RGBA {0.0f, 0.6f, 0.0f, 1.0f}
#define OSC_POSITIVE_HOVERED_RGBA {0.0f, 0.7f, 0.0f, 1.0f}

// theme color for neutral things (importing, editing)
#define OSC_NEUTRAL_RGBA {0.06f, 0.53f, 0.98f, 1.00f}
#define OSC_NEUTRAL_HOVERED_RGBA {0.15f, 0.60f, 1.0f, 1.00f}

// theme color for negative things (deleting)
#define OSC_NEGATIVE_RGBA {1.0f, 0.0f, 0.0f, 1.0f}

// theme color for greyed (usually, disabled) things
#define OSC_GREYED_RGBA {0.5f, 0.5f, 0.5f, 1.0f}

// theme color for slightly greyed (usually, additional description text) things
#define OSC_SLIGHTLY_GREYED_RGBA {0.7f, 0.7f, 0.7f, 1.0f}

#define OSC_HOVERED_COMPONENT_RGBA {0.5f, 0.5f, 0.0f, 1.0f}

#define OSC_SELECTED_COMPONENT_RGBA {1.0f, 1.0f, 0.0f, 1.0f}
