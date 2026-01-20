#pragma once

#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/synchronized_value_guard.h>
#include <liboscar/utils/uid.h>

#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Model; }
namespace osc { class ModelStatePair; }

namespace osc
{
    // immutable, reference-counted handle to a "Model+State commit", which is effectively
    // what is saved upon each user action
    class ModelStateCommit final {
    public:
        ModelStateCommit(const ModelStatePair&, std::string_view message);
        ModelStateCommit(const ModelStatePair&, std::string_view message, UID parent);

        UID getID() const;
        bool hasParent() const;
        UID getParentID() const;
        CStringView getCommitMessage() const;
        SynchronizedValueGuard<const OpenSim::Model> getModel() const;
        UID getModelVersion() const;
        float getFixupScaleFactor() const;

        friend bool operator==(const ModelStateCommit&, const ModelStateCommit&) = default;
    private:
        class Impl;
        std::shared_ptr<const Impl> m_Impl;
    };
}
