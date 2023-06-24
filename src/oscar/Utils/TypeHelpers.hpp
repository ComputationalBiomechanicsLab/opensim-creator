#pragma once

namespace osc
{
    template<typename T, typename U>
    inline bool TypeIDEquals(U const& v)
    {
        return typeid(v) == typeid(T);
    }

    template<typename T, typename U>
    inline bool DerivesFrom(U const& v)
    {
        return dynamic_cast<T const*>(&v);
    }
}
