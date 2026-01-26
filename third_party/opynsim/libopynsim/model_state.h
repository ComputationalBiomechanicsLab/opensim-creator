#pragma once

#include <liboscar/utilities/copy_on_upd_ptr.h>

namespace opyn { class Model; }
namespace SimTK { class State; }

namespace opyn
{
    class ModelState final {
    public:
        const SimTK::State& simbody_state() const;
    private:
        friend class Model;
        explicit ModelState(SimTK::State&&);

        class Impl;
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
