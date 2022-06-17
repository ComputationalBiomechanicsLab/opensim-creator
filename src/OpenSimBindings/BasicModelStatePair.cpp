#include "BasicModelStatePair.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Perf.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

class osc::BasicModelStatePair::Impl final {
public:

    explicit Impl(VirtualModelStatePair const& p) : Impl{p.getModel(), p.getState()}
    {
        m_FixupScaleFactor = p.getFixupScaleFactor();
    }

    Impl(OpenSim::Model const& m, SimTK::State const& st) :
        m_Model(std::make_unique<OpenSim::Model>(m))
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
        m_Model->updWorkingState() = st;
        m_Model->updWorkingState().invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        m_Model->realizeReport(m_Model->updWorkingState());
    }

    Impl(Impl const& o) :
        m_Model{std::make_unique<OpenSim::Model>(*o.m_Model)}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
        m_Model->updWorkingState() = o.m_Model->getWorkingState();
    }

    std::unique_ptr<Impl> clone()
    {
        return std::make_unique<Impl>(*this);
    }

    OpenSim::Model const& getModel() const
    {
        return *m_Model;
    }

    SimTK::State const& getState() const
    {
        return m_Model->getWorkingState();
    }

    float getFixupScaleFactor() const
    {
        return m_FixupScaleFactor;
    }

    void setFixupScaleFactor(float v)
    {
        m_FixupScaleFactor = std::move(v);
    }

private:
    std::unique_ptr<OpenSim::Model> m_Model;
    float m_FixupScaleFactor = 1.0f;
};


// public API

osc::BasicModelStatePair::BasicModelStatePair(VirtualModelStatePair const& p) :
    m_Impl{new Impl{p}}
{
}
osc::BasicModelStatePair::BasicModelStatePair(OpenSim::Model const& model, SimTK::State const& state) :
    m_Impl{new Impl{model, state}}
{
}
osc::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair const&) = default;
osc::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair&&) noexcept = default;
osc::BasicModelStatePair& osc::BasicModelStatePair::operator=(BasicModelStatePair const&) = default;
osc::BasicModelStatePair& osc::BasicModelStatePair::operator=(BasicModelStatePair&&) noexcept = default;
osc::BasicModelStatePair::~BasicModelStatePair() noexcept = default;

OpenSim::Model const& osc::BasicModelStatePair::getModel() const
{
    return m_Impl->getModel();
}

SimTK::State const& osc::BasicModelStatePair::getState() const
{
    return m_Impl->getState();
}

float osc::BasicModelStatePair::getFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::BasicModelStatePair::setFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(std::move(v));
}