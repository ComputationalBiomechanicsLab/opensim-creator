#include "basic_model_state_pair.h"

#include <libopynsim/utilities/open_sim_helpers.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <memory>

using namespace opyn;

class opyn::BasicModelStatePair::Impl final {
public:

    Impl() :
        model_{std::make_unique<OpenSim::Model>()}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
    }

    explicit Impl(const ModelStatePair& p) :
        Impl{p.get_model(), p.get_state(), p.get_fixup_scale_factor()}
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
        const OpenSim::Model& model,
        const SimTK::State& state,
        float fixup_scale_factor) :

        model_(std::make_unique<OpenSim::Model>(model)),
        fixup_scale_factor_{fixup_scale_factor}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
        model_->updWorkingState() = state;
        model_->updWorkingState().invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);
        model_->realizeReport(model_->updWorkingState());
    }

    Impl(const Impl& o) :
        model_{std::make_unique<OpenSim::Model>(*o.model_)},
        fixup_scale_factor_{o.fixup_scale_factor_}
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

    std::unique_ptr<Impl> clone()
    {
        return std::make_unique<Impl>(*this);
    }

    const OpenSim::Model& get_model() const
    {
        return *model_;
    }

    const SimTK::State& get_state() const
    {
        return model_->getWorkingState();
    }

    float get_fixup_scale_factor() const
    {
        return fixup_scale_factor_;
    }

    void set_fixup_scale_factor(float v)
    {
        fixup_scale_factor_ = v;
    }
private:
    std::unique_ptr<OpenSim::Model> model_;
    float fixup_scale_factor_ = 1.0f;
};

opyn::BasicModelStatePair::BasicModelStatePair() :
    impl_{std::make_unique<Impl>()}
{}

opyn::BasicModelStatePair::BasicModelStatePair(const ModelStatePair& p) :
    impl_{std::make_unique<Impl>(p)}
{}

opyn::BasicModelStatePair::BasicModelStatePair(const std::filesystem::path& p) :
    impl_{std::make_unique<Impl>(p)}
{}
opyn::BasicModelStatePair::BasicModelStatePair(OpenSim::Model&& model) :
    impl_{std::make_unique<Impl>(std::move(model))}
{}

opyn::BasicModelStatePair::BasicModelStatePair(const OpenSim::Model& model, const SimTK::State& state) :
    impl_{std::make_unique<Impl>(model, state)}
{}
opyn::BasicModelStatePair::BasicModelStatePair(const BasicModelStatePair&) = default;
opyn::BasicModelStatePair::BasicModelStatePair(BasicModelStatePair&&) noexcept = default;
opyn::BasicModelStatePair& opyn::BasicModelStatePair::operator=(const BasicModelStatePair&) = default;
opyn::BasicModelStatePair& opyn::BasicModelStatePair::operator=(BasicModelStatePair&&) noexcept = default;
opyn::BasicModelStatePair::~BasicModelStatePair() noexcept = default;

const OpenSim::Model& opyn::BasicModelStatePair::impl_get_model() const
{
    return impl_->get_model();
}

const SimTK::State& opyn::BasicModelStatePair::impl_get_state() const
{
    return impl_->get_state();
}

float opyn::BasicModelStatePair::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void opyn::BasicModelStatePair::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}
