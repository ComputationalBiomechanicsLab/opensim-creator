#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

namespace osc
{
    enum class TPSDocumentCSVFlags {
        None     = 0,
        NoHeader = 1<<0,
        NoNames  = 1<<1,
    };

    constexpr TPSDocumentCSVFlags operator|(TPSDocumentCSVFlags lhs, TPSDocumentCSVFlags rhs)
    {
        return static_cast<TPSDocumentCSVFlags>(osc::to_underlying(lhs) | osc::to_underlying(rhs));
    }

    constexpr bool operator&(TPSDocumentCSVFlags lhs, TPSDocumentCSVFlags rhs)
    {
        return (osc::to_underlying(lhs) & osc::to_underlying(rhs)) != 0;
    }
}
