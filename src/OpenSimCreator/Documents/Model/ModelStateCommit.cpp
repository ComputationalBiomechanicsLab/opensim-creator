#include "ModelStateCommit.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/SynchronizedValueGuard.h>
#include <oscar/Utils/UID.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

using namespace osc;

class osc::ModelStateCommit::Impl final {
public:
    Impl(const IModelStatePair& msp, std::string_view message) :
        Impl{msp, message, UID::empty()}
    {}

    Impl(const IModelStatePair& msp, std::string_view message, UID parent) :
        m_MaybeParentID{parent},
        m_CommitTime{std::chrono::system_clock::now()},
        m_Model{std::make_unique<OpenSim::Model>(msp.getModel())},
        m_ModelVersion{msp.getModelVersion()},
        m_FixupScaleFactor{msp.getFixupScaleFactor()},
        m_CommitMessage{message}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
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

    CStringView getCommitMessage() const
    {
        return m_CommitMessage;
    }

    SynchronizedValueGuard<const OpenSim::Model> getModel() const
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


osc::ModelStateCommit::ModelStateCommit(const IModelStatePair& p, std::string_view message) :
    m_Impl{std::make_shared<Impl>(p, message)}
{}
osc::ModelStateCommit::ModelStateCommit(const IModelStatePair& p, std::string_view message, UID parent) :
    m_Impl{std::make_shared<Impl>(p, message, parent)}
{}

UID osc::ModelStateCommit::getID() const
{
    return m_Impl->getID();
}

bool osc::ModelStateCommit::hasParent() const
{
    return m_Impl->hasParent();
}

UID osc::ModelStateCommit::getParentID() const
{
    return m_Impl->getParentID();
}

std::chrono::system_clock::time_point osc::ModelStateCommit::getCommitTime() const
{
    return m_Impl->getCommitTime();
}

CStringView osc::ModelStateCommit::getCommitMessage() const
{
    return m_Impl->getCommitMessage();
}

SynchronizedValueGuard<const OpenSim::Model> osc::ModelStateCommit::getModel() const
{
    return m_Impl->getModel();
}

UID osc::ModelStateCommit::getModelVersion() const
{
    return m_Impl->getModelVersion();
}

float osc::ModelStateCommit::getFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}
