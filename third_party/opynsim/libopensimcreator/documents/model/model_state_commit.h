#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/synchronized_value_guard.h>
#include <liboscar/utilities/uid.h>

#include <memory>
#include <string_view>

namespace OpenSim
{
    class ComponentPath;
}
namespace OpenSim
{
    class Model;
}
namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    // immutable, reference-counted handle to a "Model+State commit", which is effectively
    // what is saved upon each user action
    class ModelStateCommit final {
    public:
        ModelStateCommit(const opyn::ModelStatePair&, std::string_view message);
        ModelStateCommit(const opyn::ModelStatePair&, std::string_view message, UID parent);

        UID get_id() const;
        bool has_parent() const;
        UID get_parent_id() const;
        CStringView get_commit_message() const;
        SynchronizedValueGuard<const OpenSim::Model> get_model() const;
        UID get_model_version() const;
        float get_fixup_scale_factor() const;

        friend bool operator==(const ModelStateCommit&, const ModelStateCommit&) = default;

    private:
        class Impl;
        std::shared_ptr<const Impl> impl_;
    };
}
