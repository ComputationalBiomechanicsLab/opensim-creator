#include "ModelStateCommit.hpp"

#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Utils/SynchronizedValue.hpp"
#include "src/Utils/UID.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

class osc::ModelStateCommit::Impl final {
public:
	Impl(VirtualConstModelStatePair const& msp, std::string_view message) :
		Impl{msp, message, UID::empty()}
	{
	}

	Impl(VirtualConstModelStatePair const& msp, std::string_view message, UID parent) :
		m_AccessMutex{},
		m_ID{},
		m_MaybeParentID{std::move(parent)},
		m_CommitTime{std::chrono::system_clock::now()},
		m_Model{std::make_unique<OpenSim::Model>(msp.getModel())},
		m_ModelVersion{msp.getModelVersion()},
		m_FixupScaleFactor{msp.getFixupScaleFactor()},
		m_CommitMessage{std::move(message)}
	{
		osc::InitializeModel(*m_Model);
		osc::InitializeState(*m_Model);
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

	SynchronizedValueGuard<OpenSim::Model const> getModel() const
	{
		return {m_AccessMutex, *m_Model};
	}

	UID getModelVersion() const
	{
		return m_ModelVersion;
	}

	float getFixupScaleFactor() const
	{
		return m_FixupScaleFactor;
	}

private:
	mutable std::mutex m_AccessMutex;
	UID m_ID;
	UID m_MaybeParentID;
	std::chrono::system_clock::time_point m_CommitTime;
	std::unique_ptr<OpenSim::Model> m_Model;
	UID m_ModelVersion;
	float m_FixupScaleFactor;
	std::string m_CommitMessage;
};


// public API (PIMPL)

osc::ModelStateCommit::ModelStateCommit(VirtualConstModelStatePair const& p, std::string_view message) :
	m_Impl{std::make_shared<Impl>(p, std::move(message))}
{
}

osc::ModelStateCommit::ModelStateCommit(VirtualConstModelStatePair const& p, std::string_view message, UID parent) :
	m_Impl{std::make_shared<Impl>(p, std::move(message), std::move(parent))}
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

osc::SynchronizedValueGuard<OpenSim::Model const> osc::ModelStateCommit::getModel() const
{
	return m_Impl->getModel();
}

osc::UID osc::ModelStateCommit::getModelVersion() const
{
	return m_Impl->getModelVersion();
}

float osc::ModelStateCommit::getFixupScaleFactor() const
{
	return m_Impl->getFixupScaleFactor();
}
