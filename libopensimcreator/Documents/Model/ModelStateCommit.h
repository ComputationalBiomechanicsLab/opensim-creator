#pragma once

#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/SynchronizedValueGuard.h>
#include <liboscar/utils/UID.h>

#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Model; }
namespace osc { class IModelStatePair; }

namespace osc
{
    // immutable, reference-counted handle to a "Model+State commit", which is effectively
    // what is saved upon each user action
    class ModelStateCommit final {
    public:
        ModelStateCommit(const IModelStatePair&, std::string_view message);
        ModelStateCommit(const IModelStatePair&, std::string_view message, UID parent);

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
