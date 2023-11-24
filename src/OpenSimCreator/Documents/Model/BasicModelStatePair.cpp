#include "BasicModelStatePair.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <memory>
#include <utility>

class osc::BasicModelStatePair::Impl final {
public:

    Impl() :
        m_Model{std::make_unique<OpenSim::Model>()}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
    }

    explicit Impl(VirtualModelStatePair const& p) :
        Impl{p.getModel(), p.getState(), p.getFixupScaleFactor()}
    {
    }

    Impl(OpenSim::Model const& m, SimTK::State const& st) :
        Impl{m, st, 1.0f}
    {
    }

    Impl(OpenSim::Model const& m, SimTK::State const& st, float fixupScaleFactor) :
        m_Model(std::make_unique<OpenSim::Model>(m)),
        m_FixupScaleFactor{fixupScaleFactor}
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
    Impl(Impl&&) noexcept = default;
    Impl& operator=(Impl const&) = delete;
    Impl& operator=(Impl&&) noexcept = default;
    ~Impl() noexcept = default;

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
        m_FixupScaleFactor = v;
    }

private:
    std::unique_ptr<OpenSim::Model> m_Model;
    float m_FixupScaleFactor = 1.0f;
};


// public API

osc::BasicModelStatePair::BasicModelStatePair() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::BasicModelStatePair::BasicModelStatePair(VirtualModelStatePair const& p) :
    m_Impl{std::make_unique<Impl>(p)}
{
}
osc::BasicModelStatePair::BasicModelStatePair(OpenSim::Model const& model, SimTK::State const& state) :
    m_Impl{std::make_unique<Impl>(model, state)}
{
}
osc::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair const&) = default;
osc::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair&&) noexcept = default;
osc::BasicModelStatePair& osc::BasicModelStatePair::operator=(BasicModelStatePair const&) = default;
osc::BasicModelStatePair& osc::BasicModelStatePair::operator=(BasicModelStatePair&&) noexcept = default;
osc::BasicModelStatePair::~BasicModelStatePair() noexcept = default;

OpenSim::Model const& osc::BasicModelStatePair::implGetModel() const
{
    return m_Impl->getModel();
}

SimTK::State const& osc::BasicModelStatePair::implGetState() const
{
    return m_Impl->getState();
}

float osc::BasicModelStatePair::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::BasicModelStatePair::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}
