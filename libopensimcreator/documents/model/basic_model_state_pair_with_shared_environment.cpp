#include "basic_model_state_pair_with_shared_environment.h"

#include <libopensimcreator/documents/model/environment.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <memory>

using namespace osc;

class osc::BasicModelStatePairWithSharedEnvironment::Impl final {
public:

    Impl() :
        m_Model{std::make_unique<OpenSim::Model>()}
    {
        opyn::initialize_model(*m_Model);
        opyn::initialize_state(*m_Model);
    }

    explicit Impl(const ModelStatePairWithSharedEnvironment& p) :
        Impl{p.get_model(), p.get_state(), p.get_fixup_scale_factor(), p.tryUpdEnvironment()}
    {}

    explicit Impl(const std::filesystem::path& osimPath) :
        m_Model{opyn::load_model(osimPath)}
    {
        opyn::initialize_model(*m_Model);
        opyn::initialize_state(*m_Model);
    }

    explicit Impl(OpenSim::Model&& model) :
        m_Model{std::make_unique<OpenSim::Model>(std::move(model))}
    {
        opyn::initialize_model(*m_Model);
        opyn::initialize_state(*m_Model);
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
        opyn::initialize_model(*m_Model);
        opyn::initialize_state(*m_Model);
        m_Model->updWorkingState() = st;
        m_Model->updWorkingState().invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        m_Model->realizeReport(m_Model->updWorkingState());
    }

    Impl(const Impl& o) :
        m_Model{std::make_unique<OpenSim::Model>(*o.m_Model)},
        m_FixupScaleFactor{o.m_FixupScaleFactor},
        m_Environment{o.m_Environment}
    {
        opyn::initialize_model(*m_Model);
        SimTK::State& state = m_Model->initializeState();
        state = o.m_Model->getWorkingState();
        opyn::try_equilibrate_muscles_or_log_warning(*m_Model, state);
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

    std::shared_ptr<Environment> implUpdAssociatedEnvironment()
    {
        return m_Environment;
    }
private:
    std::unique_ptr<OpenSim::Model> m_Model;
    float m_FixupScaleFactor = 1.0f;
    std::shared_ptr<Environment> m_Environment = std::make_shared<Environment>();
};


osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment() :
    m_Impl{std::make_unique<Impl>()}
{}

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(const ModelStatePairWithSharedEnvironment& p) :
    m_Impl{std::make_unique<Impl>(p)}
{}

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(const std::filesystem::path& p) :
    m_Impl{std::make_unique<Impl>(p)}
{}
osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(OpenSim::Model&& model) :
    m_Impl{std::make_unique<Impl>(std::move(model))}
{}

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(const OpenSim::Model& model, const SimTK::State& state) :
    m_Impl{std::make_unique<Impl>(model, state)}
{}
osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(const BasicModelStatePairWithSharedEnvironment&) = default;
osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(BasicModelStatePairWithSharedEnvironment&&) noexcept = default;
osc::BasicModelStatePairWithSharedEnvironment& osc::BasicModelStatePairWithSharedEnvironment::operator=(const BasicModelStatePairWithSharedEnvironment&) = default;
osc::BasicModelStatePairWithSharedEnvironment& osc::BasicModelStatePairWithSharedEnvironment::operator=(BasicModelStatePairWithSharedEnvironment&&) noexcept = default;
osc::BasicModelStatePairWithSharedEnvironment::~BasicModelStatePairWithSharedEnvironment() noexcept = default;

const OpenSim::Model& osc::BasicModelStatePairWithSharedEnvironment::impl_get_model() const
{
    return m_Impl->getModel();
}

const SimTK::State& osc::BasicModelStatePairWithSharedEnvironment::impl_get_state() const
{
    return m_Impl->getState();
}

float osc::BasicModelStatePairWithSharedEnvironment::impl_get_fixup_scale_factor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::BasicModelStatePairWithSharedEnvironment::impl_set_fixup_scale_factor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}

std::shared_ptr<Environment> osc::BasicModelStatePairWithSharedEnvironment::implUpdAssociatedEnvironment() const
{
    return m_Impl->implUpdAssociatedEnvironment();
}
