#pragma once

#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Utils/UID.hpp"

#include <chrono>
#include <memory>
#include <string_view>

namespace OpenSim
{
	class Component;
	class Model;
}

namespace osc
{
	class AutoFinalizingModelStatePair;
}

namespace SimTK
{
	class State;
}

namespace osc
{
	// immutable, reference-counted handle to a "Model+State commit", which is effectively
	// what is saved upon each user action
	class ModelStateCommit : public VirtualConstModelStatePair {
	public:
		ModelStateCommit(AutoFinalizingModelStatePair const&, std::string_view message);
		ModelStateCommit(AutoFinalizingModelStatePair const&, std::string_view message, UID parent);
		ModelStateCommit(ModelStateCommit const&);
		ModelStateCommit(ModelStateCommit&&) noexcept;
		ModelStateCommit& operator=(ModelStateCommit const&);
		ModelStateCommit& operator=(ModelStateCommit&&) noexcept;
		~ModelStateCommit() noexcept override;

		UID getID() const;
		bool hasParent() const;
		UID getParentID() const;
		std::chrono::system_clock::time_point getCommitTime() const;

		OpenSim::Model const& getModel() const override;
		SimTK::State const& getState() const override;
		UID getModelVersion() const override;
		UID getStateVersion() const override;
		OpenSim::Component const* getSelected() const override;
		OpenSim::Component const* getHovered() const override;
		OpenSim::Component const* getIsolated() const override;
		float getFixupScaleFactor() const override;

		class Impl;
	private:
		std::shared_ptr<Impl> m_Impl;
	};
}