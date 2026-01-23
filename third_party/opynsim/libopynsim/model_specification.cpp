#include "model_specification.h"

#include <OpenSim/Simulation/Model/Model.h>

using namespace opyn;

class opyn::ModelSpecification::Impl final {
public:
    explicit Impl(const std::filesystem::path& osim_path) :
        model_{osc::make_cow<OpenSim::Model>(osim_path.string())}
    {}

    Model compile() const { return Model{*model_}; }
private:
    osc::CopyOnUpdPtr<OpenSim::Model> model_;
};

opyn::ModelSpecification opyn::ModelSpecification::from_osim_file(const std::filesystem::path& osim_path)
{
    return ModelSpecification{osc::make_cow<Impl>(osim_path)};
}

opyn::ModelSpecification::ModelSpecification(osc::CopyOnUpdPtr<Impl> impl) :
    impl_{std::move(impl)}
{}

Model opyn::ModelSpecification::compile() const { return impl_->compile(); }
