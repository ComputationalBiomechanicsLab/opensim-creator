#include "model_state_commit.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/synchronized_value_guard.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

using namespace osc;

class osc::ModelStateCommit::Impl final {
public:
    Impl(const opyn::ModelStatePair& msp, std::string_view message) :
        Impl{msp, message, UID::empty()}
    {}

    Impl(const opyn::ModelStatePair& msp, std::string_view message, UID parent) :
        maybe_parent_id_{parent},
        model_{std::make_unique<OpenSim::Model>(msp.get_model())},
        model_version_{msp.get_model_version()},
        fixup_scale_factor_{msp.get_fixup_scale_factor()},
        commit_message_{message}
    {
        opyn::initialize_model(*model_);
        opyn::initialize_state(*model_);
    }

    UID get_id() const { return id_; }

    bool has_parent() const { return maybe_parent_id_ != UID::empty(); }

    UID get_parent_id() const { return maybe_parent_id_; }

    CStringView get_commit_message() const { return commit_message_; }

    SynchronizedValueGuard<const OpenSim::Model> get_model() const
    {
        return {access_mutex_, *model_};
    }

    UID get_model_version() const { return model_version_; }

    float get_fixup_scale_factor() const { return fixup_scale_factor_; }

private:
    mutable std::mutex access_mutex_;
    UID id_;
    UID maybe_parent_id_;
    std::unique_ptr<OpenSim::Model> model_;
    UID model_version_;
    float fixup_scale_factor_;
    std::string commit_message_;
};

osc::ModelStateCommit::ModelStateCommit(const opyn::ModelStatePair& p, std::string_view message) :
    impl_{std::make_shared<Impl>(p, message)}
{}
osc::ModelStateCommit::ModelStateCommit(
    const opyn::ModelStatePair& p, std::string_view message, UID parent) :

    impl_{std::make_shared<Impl>(p, message, parent)}
{}

UID osc::ModelStateCommit::get_id() const
{
    return impl_->get_id();
}

bool osc::ModelStateCommit::has_parent() const
{
    return impl_->has_parent();
}

UID osc::ModelStateCommit::get_parent_id() const
{
    return impl_->get_parent_id();
}

CStringView osc::ModelStateCommit::get_commit_message() const
{
    return impl_->get_commit_message();
}

SynchronizedValueGuard<const OpenSim::Model> osc::ModelStateCommit::get_model() const
{
    return impl_->get_model();
}

UID osc::ModelStateCommit::get_model_version() const
{
    return impl_->get_model_version();
}

float osc::ModelStateCommit::get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}
