#pragma once

#include <algorithm>

namespace osc {

    // remove all elements `e` in `Container` `c` for which `p(e)` returns `true`
    template<typename Container, typename UnaryPredicate>
    void remove_erase(Container& c, UnaryPredicate p) {
        auto it = std::remove_if(c.begin(), c.end(), p);
        c.erase(it, c.end());
    }

}
