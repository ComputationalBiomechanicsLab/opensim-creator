#include "model_warper.h"

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/solvers/model_warper/model_warper_v3_document.h>
#include <libopynsim/solvers/model_warper/scaling_document_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/model.h>
#include <libopynsim/model_specification.h>
#include <libopynsim/model_state.h>
#include <libopynsim/model_state_stage.h>

#include <liboscar/formats/obj.h>
#include <liboscar/utilities/copy_on_upd_shared_value.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Geometry.h>

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace opyn;

namespace
{
    /// Utility adaptor that adapts OPynSim's `Model`/`ModelState` to
    /// the `ModelStatePair` API.
    class OPynSimModelStatePair : public ModelStatePair {
    public:
        explicit OPynSimModelStatePair(Model model, ModelState model_state) :
            model_{std::move(model)},
            model_state_{std::move(model_state)}
        {}
    private:
        const OpenSim::Model& implGetModel() const override { return model_.open_sim_model(); }
        const SimTK::State& implGetState()   const override { return model_state_.simbody_state(); }

        Model model_;
        ModelState model_state_;
    };
}

class opyn::ModelWarper::Impl {
public:
    Impl() = default;
    explicit Impl(const std::filesystem::path& source) : warping_document_{source} {}

    size_t num_scaling_steps() const { return warping_document_.getNumScalingSteps(); }
    size_t num_scaling_parameters() const { return warping_document_.getNumScalingParameters(); }

    void warp_to_osim_file(
        const ModelSpecification& model_specification,
        const std::filesystem::path& osim_output_path,
        const std::filesystem::path& warped_geometry_directory,
        bool bake_station_defined_frames) const
    {
        ScalingCache scaling_cache;
        Model model = model_specification.compile();
        ModelState state = model.initial_state(ModelStateStage::report);
        OPynSimModelStatePair msp{model, state};

        // Collect validation issues, throw an exception if there are any.
        const auto validation_issues = collect_scaling_validation_issues(scaling_cache, msp);
        if (not validation_issues.empty()) {
            throw std::runtime_error{create_validation_issues_error_message(validation_issues)};
        }

        // Perform the model warp.
        OpenSim::Model warped_model = warp_model(scaling_cache, msp);
        warped_model.setInputFileName(osim_output_path.string());

        // Apply any specific/hacky fixups (e.g. flushing in-memory meshes to disk) to the warped model.
        apply_fixups_to_model(
            msp.getModel(),
            warped_model,
            osim_output_path,
            warped_geometry_directory,
            bake_station_defined_frames
        );

        // Write the model to the final output path
        // TODO: might require calling `setInputFileName` first.
        warped_model.print(osim_output_path.string());
    }
private:
    /// Applies fixups to `model` in-place.
    ///
    /// Fixups are usually hacky/custom steps that don't fit the `ScalingStep`
    /// architecture but are necessary to deliver a compatible/working
    /// `OpenSim::Model` result.
    void apply_fixups_to_model(
        const OpenSim::Model& source_model,
        OpenSim::Model& warped_model,
        const std::filesystem::path& osim_output_path,
        const std::filesystem::path& warped_geometry_directory,
        bool bake_station_defined_frames) const
    {
        flush_in_memory_meshes_to_disk(source_model, warped_model, osim_output_path, warped_geometry_directory);
        if (bake_station_defined_frames) {
            BakeStationDefinedFrames(warped_model);
        }
    }

    void flush_in_memory_meshes_to_disk(
        const OpenSim::Model& source_model,
        OpenSim::Model& warped_model,
        const std::filesystem::path& osim_output_path,
        const std::filesystem::path& warped_geometry_directory) const
    {
        auto in_memory_meshes = warped_model.getComponentList<InMemoryMesh>();
        if (in_memory_meshes.begin() == in_memory_meshes.end()) {
            return;  // Model contains no `InMemoryMesh`es.
        }

        // `warped_geometry_directory` is what's written _in_ the osim, but its location
        // on-disk depends on the disk location of the osim.
        const std::filesystem::path& property_path = warped_geometry_directory;
        const std::filesystem::path on_disk_dir = osim_output_path.parent_path() / warped_geometry_directory;

        // Ensure the output directory exists.
        if (not std::filesystem::exists(on_disk_dir)) {
            std::filesystem::create_directories(on_disk_dir);
        }

        // Flush each `InMemoryMesh` to disk (opensim-creator#1003).
        for (const InMemoryMesh& in_memory_mesh : in_memory_meshes) {
            // Compute warped mesh filename and path.
            const auto& input_mesh = source_model.getComponent<OpenSim::Mesh>(in_memory_mesh.getAbsolutePath());
            auto warped_mesh_filename = std::filesystem::path{input_mesh.get_mesh_file()}.filename();
            warped_mesh_filename.replace_extension(".obj");
            const auto warped_mesh_abs_path = std::filesystem::weakly_canonical(on_disk_dir / warped_mesh_filename);

            // Write in-memory warped mesh data to disk as an OBJ file.
            {
                std::ofstream obj_stream{warped_mesh_abs_path, std::ios::trunc};
                obj_stream.exceptions(std::ios::badbit | std::ios::failbit);
                osc::OBJ::write(obj_stream, in_memory_mesh.getOscMesh(), osc::OBJMetadata{"osc-model-warper"});
            }

            // Replace `InMemoryMesh` with a standard `OpenSim::Mesh`.
            auto& mutable_in_memory_mesh = warped_model.updComponent<InMemoryMesh>(in_memory_mesh.getAbsolutePath());
            auto opensim_mesh = std::make_unique<OpenSim::Mesh>();
            opensim_mesh->set_mesh_file(property_path.string());
            OverwriteGeometry(warped_model, mutable_in_memory_mesh, std::move(opensim_mesh));
        }
    }

    /// Returns a warped version of `source_model` - assumes there are no validation issues.
    OpenSim::Model warp_model(ScalingCache& scaling_cache, const ModelStatePair& msp) const
    {
        OSC_ASSERT(not has_validation_issues(scaling_cache, msp));

        OpenSim::Model rv = msp.getModel();
        rv.clearConnections();
        InitializeModel(rv);
        InitializeState(rv);

        if (not warping_document_.hasScalingSteps()) {
            return rv;  // No `ScalingStep`s, nothing to do.
        }

        // Apply each `ScalingStep` one-by-one.
        const ScalingParameters scaling_parameters = warping_document_.getEffectiveScalingParameters();
        for (const auto& scaling_step : warping_document_.getComponentList<ScalingStep>()) {
            scaling_step.applyScalingStep(scaling_cache, scaling_parameters, msp.getModel(), rv);
        }

        // Ensure the modified model is initialized before returning.
        InitializeModel(rv);
        InitializeState(rv);

        return rv;
    }

    /// Returns a human-readable error message representation of `validation_messages`.
    std::string create_validation_issues_error_message(
        const std::vector<ScalingDocumentValidationMessage>& validation_messages) const
    {
        std::string rv = "Cannot warp `model_specification` due to validation errors:\n";
        for (const auto& validation_message : validation_messages) {
            std::format_to(
                std::back_inserter(rv),
                "- {} ({})",
                validation_message.payload.getMessage(),
                validation_message.sourceScalingStepAbsPath.toString()
            );
        }
        return rv;
    }

    /// Returns all validation messages from all enabled `ScalingStep`s.
    std::vector<ScalingDocumentValidationMessage> collect_scaling_validation_issues(
        ScalingCache& scaling_cache,
        const ModelStatePair& msp) const
    {
        std::vector<ScalingDocumentValidationMessage> rv;

        if (not warping_document_.hasScalingSteps()) {
            return rv;  // No scaling steps, no validation messages.
        }

        const ScalingParameters scaling_params = warping_document_.getEffectiveScalingParameters();
        for (const auto& scaling_step : warping_document_.getComponentList<ScalingStep>()) {
            if (not scaling_step.get_enabled()) {
                continue;  // Only enabled `ScalingStep`s are validated.
            }

            const auto messages = scaling_step.validate(scaling_cache, scaling_params, msp);
            rv.reserve(rv.size() + messages.size());
            for (auto& message : messages) {
                rv.push_back(ScalingDocumentValidationMessage{
                    .sourceScalingStepAbsPath = scaling_step.getAbsolutePath(),
                    .payload = std::move(message),
                });
            }
        }

        return rv;
    }

    /// Returns `true` if `msp` has validation issues.
    bool has_validation_issues(ScalingCache& scaling_cache, const ModelStatePair& msp) const
    {
        return not collect_scaling_validation_issues(scaling_cache, msp).empty();
    }

    ModelWarperV3Document warping_document_;
};

opyn::ModelWarper::ModelWarper() :
    impl_{osc::make_cowv<Impl>()}
{}
opyn::ModelWarper::ModelWarper(const std::filesystem::path& source) :
    impl_{osc::make_cowv<Impl>(source)}
{}

size_t opyn::ModelWarper::num_scaling_steps() const      { return impl_->num_scaling_steps(); }
size_t opyn::ModelWarper::num_scaling_parameters() const { return impl_->num_scaling_parameters(); }
void opyn::ModelWarper::warp_to_osim_file(
    const ModelSpecification& model_specification,
    const std::filesystem::path& osim_output_path,
    const std::filesystem::path& warped_geometry_directory,
    bool bake_station_defined_frames) const
{
    impl_->warp_to_osim_file(
        model_specification,
        osim_output_path,
        warped_geometry_directory,
        bake_station_defined_frames
    );
}
