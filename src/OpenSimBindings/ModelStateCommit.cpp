#include "ModelStateCommit.hpp"

#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/Utils/UID.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

class osc::ModelStateCommit::Impl final {
public:
	Impl(AutoFinalizingModelStatePair const& msp, std::string_view message) :
		m_ModelState{msp},
		m_CommitMessage{std::move(message)}
	{
	}

	Impl(AutoFinalizingModelStatePair const& msp, std::string_view message, UID parent) :
		m_MaybeParentID{std::move(parent)},
		m_ModelState{msp},
		m_CommitMessage{std::move(message)}
	{
	}

	UID getID() const
	{
		return m_ID;
	}

	bool hasParent() const
	{
		return m_MaybeParentID != UID::empty();
	}

	UID getParentID() const
	{
		return m_MaybeParentID;
	}

	std::chrono::system_clock::time_point getCommitTime() const
	{
		return m_CommitTime;
	}

	OpenSim::Model const& getModel() const
	{
		return m_ModelState.getModel();
	}

	AutoFinalizingModelStatePair const& getUiModel() const
	{
		return m_ModelState;
	}

	SimTK::State const& getState() const
	{
		return m_ModelState.getState();
	}

	UID getModelVersion() const
	{
		return m_ModelState.getModelVersion();
	}

	UID getStateVersion() const
	{
		return m_ModelState.getStateVersion();
	}

	OpenSim::Component const* getSelected() const
	{
		return m_ModelState.getSelected();
	}

	OpenSim::Component const* getHovered() const
	{
		return m_ModelState.getHovered();
	}

	OpenSim::Component const* getIsolated() const
	{
		return m_ModelState.getIsolated();
	}

	float getFixupScaleFactor() const
	{
		return m_ModelState.getFixupScaleFactor();
	}

private:
	UID m_ID;
	UID m_MaybeParentID = UID::empty();
	std::chrono::system_clock::time_point m_CommitTime = std::chrono::system_clock::now();
	AutoFinalizingModelStatePair m_ModelState;
	std::string m_CommitMessage;
};

// public API (PIMPL)

osc::ModelStateCommit::ModelStateCommit(AutoFinalizingModelStatePair const& msp, std::string_view message) :
	m_Impl{std::make_shared<Impl>(msp, std::move(message))}
{
}

osc::ModelStateCommit::ModelStateCommit(AutoFinalizingModelStatePair const& msp, std::string_view message, UID parent) :
	m_Impl{std::make_shared<Impl>(msp, std::move(message), std::move(parent))}
{
}

osc::ModelStateCommit::ModelStateCommit(ModelStateCommit const&) = default;
osc::ModelStateCommit::ModelStateCommit(ModelStateCommit&&) noexcept = default;
osc::ModelStateCommit& osc::ModelStateCommit::operator=(ModelStateCommit const&) = default;
osc::ModelStateCommit& osc::ModelStateCommit::operator=(ModelStateCommit&&) noexcept = default;
osc::ModelStateCommit::~ModelStateCommit() noexcept = default;

osc::UID osc::ModelStateCommit::getID() const
{
	return m_Impl->getID();
}

bool osc::ModelStateCommit::hasParent() const
{
	return m_Impl->hasParent();
}

osc::UID osc::ModelStateCommit::getParentID() const
{
	return m_Impl->getParentID();
}

std::chrono::system_clock::time_point osc::ModelStateCommit::getCommitTime() const
{
	return m_Impl->getCommitTime();
}

OpenSim::Model const& osc::ModelStateCommit::getModel() const
{
	return m_Impl->getModel();
}

osc::AutoFinalizingModelStatePair const& osc::ModelStateCommit::getUiModel() const
{
	return m_Impl->getUiModel();
}

SimTK::State const& osc::ModelStateCommit::getState() const
{
	return m_Impl->getState();
}

osc::UID osc::ModelStateCommit::getModelVersion() const
{
	return m_Impl->getModelVersion();
}

osc::UID osc::ModelStateCommit::getStateVersion() const
{
	return m_Impl->getStateVersion();
}

OpenSim::Component const* osc::ModelStateCommit::getSelected() const
{
	return m_Impl->getSelected();
}

OpenSim::Component const* osc::ModelStateCommit::getHovered() const
{
	return m_Impl->getHovered();
}

OpenSim::Component const* osc::ModelStateCommit::getIsolated() const
{
	return m_Impl->getIsolated();
}

float osc::ModelStateCommit::getFixupScaleFactor() const
{
	return m_Impl->getFixupScaleFactor();
}
