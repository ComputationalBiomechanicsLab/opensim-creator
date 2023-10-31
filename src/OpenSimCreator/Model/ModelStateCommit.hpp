#pragma once

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>
#include <oscar/Utils/UID.hpp>

#include <chrono>
#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace OpenSim { class Model; }
namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
    // immutable, reference-counted handle to a "Model+State commit", which is effectively
    // what is saved upon each user action
    class ModelStateCommit final {
    public:
        ModelStateCommit(VirtualConstModelStatePair const&, std::string_view message);
        ModelStateCommit(VirtualConstModelStatePair const&, std::string_view message, UID parent);
        ModelStateCommit(ModelStateCommit const&);
        ModelStateCommit(ModelStateCommit&&) noexcept;
        ModelStateCommit& operator=(ModelStateCommit const&);
        ModelStateCommit& operator=(ModelStateCommit&&) noexcept;
        ~ModelStateCommit() noexcept;

        UID getID() const;
        bool hasParent() const;
        UID getParentID() const;
        std::chrono::system_clock::time_point getCommitTime() const;
        CStringView getCommitMessage() const;
        SynchronizedValueGuard<OpenSim::Model const> getModel() const;
        UID getModelVersion() const;
        float getFixupScaleFactor() const;

        friend bool operator==(ModelStateCommit const& lhs, ModelStateCommit const& rhs)
        {
            return lhs.m_Impl == rhs.m_Impl;
        }
    private:
        class Impl;
        std::shared_ptr<Impl const> m_Impl;
    };
}
