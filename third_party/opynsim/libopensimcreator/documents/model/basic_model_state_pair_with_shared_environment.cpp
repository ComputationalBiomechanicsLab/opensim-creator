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
        model_{std::make_unique<OpenSim::Model>()}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
    }

    explicit Impl(const ModelStatePairWithSharedEnvironment& p) :
        Impl{p.get_model(), p.get_state(), p.get_fixup_scale_factor(), p.try_upd_environment()}
    {}

    explicit Impl(const std::filesystem::path& osim_path) :
        model_{opyn::load_model(osim_path)}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
    }

    explicit Impl(OpenSim::Model&& model) :
        model_{std::make_unique<OpenSim::Model>(std::move(model))}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
    }

    Impl(const OpenSim::Model& m, const SimTK::State& st) :
        Impl{m, st, 1.0f}
    {}

    Impl(
        const OpenSim::Model& m,
        const SimTK::State& st,
        float fixup_scale_factor,
        std::shared_ptr<Environment> environment = std::make_shared<Environment>()) :

        model_(std::make_unique<OpenSim::Model>(m)),
        fixup_scale_factor_{fixup_scale_factor},
        environment_{std::move(environment)}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
        model_->updWorkingState() = st;
        model_->updWorkingState().invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        model_->realizeReport(model_->updWorkingState());
    }

    Impl(const Impl& o) :
        model_{std::make_unique<OpenSim::Model>(*o.model_)},
        fixup_scale_factor_{o.fixup_scale_factor_},
        environment_{o.environment_}
    {
        opyn::initialize_model(*model_);
        SimTK::State& state = model_->initializeState();
        state = o.model_->getWorkingState();
        opyn::try_equilibrate_muscles_or_log_warning(*model_, state);
        model_->realizeDynamics(state);
    }
    Impl(Impl&&) noexcept = default;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) noexcept = default;
    ~Impl() noexcept = default;

    std::unique_ptr<Impl> clone() { return std::make_unique<Impl>(*this); }

    const OpenSim::Model& get_model() const { return *model_; }

    const SimTK::State& get_state() const { return model_->getWorkingState(); }

    float get_fixup_scale_factor() const { return fixup_scale_factor_; }

    void set_fixup_scale_factor(float v) { fixup_scale_factor_ = v; }

    std::shared_ptr<Environment> impl_upd_associated_environment() { return environment_; }

private:
    std::unique_ptr<OpenSim::Model> model_;
    float fixup_scale_factor_ = 1.0f;
    std::shared_ptr<Environment> environment_ = std::make_shared<Environment>();
};

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment() :
    impl_{std::make_unique<Impl>()}
{}

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(
    const ModelStatePairWithSharedEnvironment& p) :

    impl_{std::make_unique<Impl>(p)}
{}

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(
    const std::filesystem::path& p) :

    impl_{std::make_unique<Impl>(p)}
{}
osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(
    OpenSim::Model&& model) :

    impl_{std::make_unique<Impl>(std::move(model))}
{}

osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(
    const OpenSim::Model& model, const SimTK::State& state) :

    impl_{std::make_unique<Impl>(model, state)}
{}
osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(
    const BasicModelStatePairWithSharedEnvironment&) = default;
osc::BasicModelStatePairWithSharedEnvironment::BasicModelStatePairWithSharedEnvironment(
    BasicModelStatePairWithSharedEnvironment&&) noexcept = default;
osc::BasicModelStatePairWithSharedEnvironment& osc::BasicModelStatePairWithSharedEnvironment::
operator=(const BasicModelStatePairWithSharedEnvironment&) = default;
osc::BasicModelStatePairWithSharedEnvironment& osc::BasicModelStatePairWithSharedEnvironment::
operator=(BasicModelStatePairWithSharedEnvironment&&) noexcept = default;
osc::BasicModelStatePairWithSharedEnvironment::
    ~BasicModelStatePairWithSharedEnvironment() noexcept = default;

const OpenSim::Model& osc::BasicModelStatePairWithSharedEnvironment::impl_get_model() const
{
    return impl_->get_model();
}

const SimTK::State& osc::BasicModelStatePairWithSharedEnvironment::impl_get_state() const
{
    return impl_->get_state();
}

float osc::BasicModelStatePairWithSharedEnvironment::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void osc::BasicModelStatePairWithSharedEnvironment::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}

std::shared_ptr<Environment> osc::BasicModelStatePairWithSharedEnvironment::
    impl_upd_associated_environment() const
{
    return impl_->impl_upd_associated_environment();
}
