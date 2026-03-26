#include "basic_model_state_pair.h"

#include <libopynsim/utilities/open_sim_helpers.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <memory>

using namespace opyn;

class opyn::BasicModelStatePair::Impl final {
public:

    Impl() :
        m_Model{std::make_unique<OpenSim::Model>()}
    {
        opyn::InitializeModel(*m_Model);
        opyn::InitializeState(*m_Model);
    }

    explicit Impl(const ModelStatePair& p) :
        Impl{p.getModel(), p.getState(), p.getFixupScaleFactor()}
    {}

    explicit Impl(const std::filesystem::path& osimPath) :
        m_Model{opyn::LoadModel(osimPath)}
    {
        opyn::InitializeModel(*m_Model);
        opyn::InitializeState(*m_Model);
    }

    explicit Impl(OpenSim::Model&& model) :
        m_Model{std::make_unique<OpenSim::Model>(std::move(model))}
    {
        opyn::InitializeModel(*m_Model);
        opyn::InitializeState(*m_Model);
    }

    Impl(const OpenSim::Model& m, const SimTK::State& st) :
        Impl{m, st, 1.0f}
    {}

    Impl(
        const OpenSim::Model& m,
        const SimTK::State& st,
        float fixupScaleFactor) :

        m_Model(std::make_unique<OpenSim::Model>(m)),
        m_FixupScaleFactor{fixupScaleFactor}
    {
        opyn::InitializeModel(*m_Model);
        opyn::InitializeState(*m_Model);
        m_Model->updWorkingState() = st;
        m_Model->updWorkingState().invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        m_Model->realizeReport(m_Model->updWorkingState());
    }

    Impl(const Impl& o) :
        m_Model{std::make_unique<OpenSim::Model>(*o.m_Model)},
        m_FixupScaleFactor{o.m_FixupScaleFactor}
    {
        opyn::InitializeModel(*m_Model);
        SimTK::State& state = m_Model->initializeState();
        state = o.m_Model->getWorkingState();
        opyn::TryEquilibrateMusclesOrLogWarning(*m_Model, state);
        m_Model->realizeDynamics(state);
    }
    Impl(Impl&&) noexcept = default;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) noexcept = default;
    ~Impl() noexcept = default;

    std::unique_ptr<Impl> clone()
    {
        return std::make_unique<Impl>(*this);
    }

    const OpenSim::Model& getModel() const
    {
        return *m_Model;
    }

    const SimTK::State& getState() const
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

opyn::BasicModelStatePair::BasicModelStatePair() :
    m_Impl{std::make_unique<Impl>()}
{}

opyn::BasicModelStatePair::BasicModelStatePair(const ModelStatePair& p) :
    m_Impl{std::make_unique<Impl>(p)}
{}

opyn::BasicModelStatePair::BasicModelStatePair(const std::filesystem::path& p) :
    m_Impl{std::make_unique<Impl>(p)}
{}
opyn::BasicModelStatePair::BasicModelStatePair(OpenSim::Model&& model) :
    m_Impl{std::make_unique<Impl>(std::move(model))}
{}

opyn::BasicModelStatePair::BasicModelStatePair(const OpenSim::Model& model, const SimTK::State& state) :
    m_Impl{std::make_unique<Impl>(model, state)}
{}
opyn::BasicModelStatePair::BasicModelStatePair(const BasicModelStatePair&) = default;
opyn::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair&&) noexcept = default;
opyn::BasicModelStatePair& opyn::BasicModelStatePair::operator=(const BasicModelStatePair&) = default;
opyn::BasicModelStatePair& opyn::BasicModelStatePair::operator=(BasicModelStatePair&&) noexcept = default;
opyn::BasicModelStatePair::~BasicModelStatePair() noexcept = default;

const OpenSim::Model& opyn::BasicModelStatePair::implGetModel() const
{
    return m_Impl->getModel();
}

const SimTK::State& opyn::BasicModelStatePair::implGetState() const
{
    return m_Impl->getState();
}

float opyn::BasicModelStatePair::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void opyn::BasicModelStatePair::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}
