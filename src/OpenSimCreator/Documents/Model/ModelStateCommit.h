#pragma once

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/SynchronizedValueGuard.h>
#include <oscar/Utils/UID.h>

#include <chrono>
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
        std::chrono::system_clock::time_point getCommitTime() const;
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
