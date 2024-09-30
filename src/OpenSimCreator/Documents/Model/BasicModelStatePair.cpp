#include "BasicModelStatePair.h"

#include <OpenSimCreator/Documents/Model/Environment.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <memory>

using namespace osc;

class osc::BasicModelStatePair::Impl final {
public:

    Impl() :
        m_Model{std::make_unique<OpenSim::Model>()}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
    }

    explicit Impl(const IModelStatePair& p) :
        Impl{p.getModel(), p.getState(), p.getFixupScaleFactor(), p.tryUpdEnvironment()}
    {}

    explicit Impl(const std::filesystem::path& osimPath) :
        m_Model{std::make_unique<OpenSim::Model>(osimPath.string())}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
    }

    Impl(const OpenSim::Model& m, const SimTK::State& st) :
        Impl{m, st, 1.0f}
    {}

    Impl(
        const OpenSim::Model& m,
        const SimTK::State& st,
        float fixupScaleFactor,
        std::shared_ptr<Environment> environment = std::make_shared<Environment>()) :

        m_Model(std::make_unique<OpenSim::Model>(m)),
        m_FixupScaleFactor{fixupScaleFactor},
        m_Environment{std::move(environment)}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
        m_Model->updWorkingState() = st;
        m_Model->updWorkingState().invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        m_Model->realizeReport(m_Model->updWorkingState());
    }

    Impl(const Impl& o) :
        m_Model{std::make_unique<OpenSim::Model>(*o.m_Model)},
        m_FixupScaleFactor{o.m_FixupScaleFactor},
        m_Environment{o.m_Environment}
    {
        InitializeModel(*m_Model);
        InitializeState(*m_Model);
        m_Model->updWorkingState() = o.m_Model->getWorkingState();
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

    std::shared_ptr<Environment> implUpdAssociatedEnvironment()
    {
        return m_Environment;
    }
private:
    std::unique_ptr<OpenSim::Model> m_Model;
    float m_FixupScaleFactor = 1.0f;
    std::shared_ptr<Environment> m_Environment = std::make_shared<Environment>();
};


osc::BasicModelStatePair::BasicModelStatePair() :
    m_Impl{std::make_unique<Impl>()}
{}

osc::BasicModelStatePair::BasicModelStatePair(const IModelStatePair& p) :
    m_Impl{std::make_unique<Impl>(p)}
{}

osc::BasicModelStatePair::BasicModelStatePair(const std::filesystem::path& p) :
    m_Impl{std::make_unique<Impl>(p)}
{}

osc::BasicModelStatePair::BasicModelStatePair(const OpenSim::Model& model, const SimTK::State& state) :
    m_Impl{std::make_unique<Impl>(model, state)}
{}
osc::BasicModelStatePair::BasicModelStatePair(const BasicModelStatePair&) = default;
osc::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair&&) noexcept = default;
osc::BasicModelStatePair& osc::BasicModelStatePair::operator=(const BasicModelStatePair&) = default;
osc::BasicModelStatePair& osc::BasicModelStatePair::operator=(BasicModelStatePair&&) noexcept = default;
osc::BasicModelStatePair::~BasicModelStatePair() noexcept = default;

const OpenSim::Model& osc::BasicModelStatePair::implGetModel() const
{
    return m_Impl->getModel();
}

const SimTK::State& osc::BasicModelStatePair::implGetState() const
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

std::shared_ptr<Environment> osc::BasicModelStatePair::implUpdAssociatedEnvironment() const
{
    return m_Impl->implUpdAssociatedEnvironment();
}
