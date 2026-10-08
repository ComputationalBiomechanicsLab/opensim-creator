#pragma once

#include <libopynsim/documents/model/versioned_component_accessor.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_parameter_declaration.h>
#include <libopynsim/solvers/model_warper/scaling_parameter_override.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Property.h>

#include <filesystem>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace opyn
{
    // Top-level document that describes a sequence of `ScalingStep`s that can be applied to
    // the source model in order to yield a scaled model.
    class ModelWarperV3Document final :
        public OpenSim::Component,
        public VersionedComponentAccessor {
        OpenSim_DECLARE_CONCRETE_OBJECT(ModelWarperV3Document, OpenSim::Component)

        OpenSim_DECLARE_LIST_PROPERTY(scaling_parameter_overrides, ScalingParameterOverride, "A sequence of `ScalingParameterOverride`s that should be used in place of the default values used by the `ScalingStep`s.");
    public:
        explicit ModelWarperV3Document()
        {
            construct_properties();
            finalizeFromProperties();
            finalizeConnections(*this);
        }

        /// Constructs a `ModelWarperV3Document` by loading its contents from `source`.
        explicit ModelWarperV3Document(const std::filesystem::path& source) :
            Component{source.string(), false}  // Associate `Component` with `source`.
        {
            construct_properties();
            updateFromXMLDocument();
            finalizeFromProperties();
            finalizeConnections(*this);
        }

        bool has_scaling_steps() const
        {
            if (getNumImmediateSubcomponents() == 0) {
                return false;
            }
            const auto lst = getComponentList<ScalingStep>();
            return lst.begin() != lst.end();
        }

        size_t get_num_scaling_steps() const
        {
            if (getNumImmediateSubcomponents() == 0) {
                return 0;
            }
            const auto lst = getComponentList<ScalingStep>();
            return std::distance(lst.begin(), lst.end());
        }

        auto iterate_scaling_steps() const
        {
            return getComponentList<ScalingStep>();
        }

        void add_scaling_step(std::unique_ptr<ScalingStep> step)
        {
            addComponent(step.release());
            clearConnections();
            finalizeConnections(*this);
            finalizeFromProperties();
        }

        bool remove_scaling_step(ScalingStep& step)
        {
            if (not step.hasOwner()) {
                return false;
            }
            if (&step.getOwner() != this) {
                return false;
            }

            removeComponent(&step);
            clearConnections();
            finalizeConnections(*this);
            finalizeFromProperties();
            return true;
        }

        bool has_scaling_parameters() const
        {
            if (not has_scaling_steps()) {
                return false;
            }
            for (const ScalingStep& step : iterate_scaling_steps()) {
                bool called = false;
                step.for_each_scaling_parameter_declaration([&called](const ScalingParameterDeclaration&) { called = true; });
                if (called) {
                    return true;
                }
            }
            return false;
        }

        size_t get_num_scaling_parameters() const
        {
            return get_effective_scaling_parameters().size();
        }

        ScalingParameters get_effective_scaling_parameters() const
        {
            ScalingParameters rv;
            if (getNumImmediateSubcomponents() == 0) {
                return rv;
            }

            // Get/merge values from the scaling steps
            for (const ScalingStep& step : iterate_scaling_steps()) {
                step.for_each_scaling_parameter_declaration([&step, &rv](const ScalingParameterDeclaration& decl)
                {
                    const auto [it, inserted] = rv.try_emplace(decl.name(), decl.default_value());
                    if (not inserted and it->second != decl.default_value()) {
                        auto msg = std::format("{}: declares a scaling parameter ({}) that has the same name as another scaling parameter, but they differ: the engine cannot figure out how to rectify this difference. The parameter should have a different name, or a disambiguating prefix added to it",
                            step.getAbsolutePathString(),
                            decl.name()
                        );
                        throw std::runtime_error{std::move(msg)};
                    }
                });
            }

            // Apply overrides to the effective scaling parameters
            {
                const OpenSim::Property<ScalingParameterOverride>& overrides = getProperty_scaling_parameter_overrides();
                for (int i = 0; i < overrides.size(); ++i) {
                    const ScalingParameterOverride& o = overrides.getValue(i);
                    rv.insert_or_assign(o.get_parameter_name(), o.get_parameter_value());
                }
            }

            return rv;
        }

        bool set_scaling_parameter_override(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            mutate_scaling_parammeter_overrides_with_new_override(scalingParamName, newValue);
            finalizeFromProperties();
            return true;
        }

        void save_to(const std::filesystem::path& p) const
        {
            print(p.string());
        }

    private:
        void construct_properties()
        {
            constructProperty_scaling_parameter_overrides();
        }

        void mutate_scaling_parammeter_overrides_with_new_override(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            // First, try to find an existing override with the same name and overwrite it
            const OpenSim::Property<ScalingParameterOverride>& overrides = getProperty_scaling_parameter_overrides();
            for (int i = 0; i < overrides.size(); ++i) {
                const ScalingParameterOverride& o = overrides.getValue(i);
                if (o.get_parameter_name() == scalingParamName) {
                    updProperty_scaling_parameter_overrides().updValue(i).set_parameter_value(newValue);
                    return;  // found and overwritten
                }
            }

            // Otherwise, add a new override
            int idx = updProperty_scaling_parameter_overrides().appendValue(ScalingParameterOverride{scalingParamName, newValue});
            updProperty_scaling_parameter_overrides().updValue(idx).set_parameter_name(scalingParamName);
            updProperty_scaling_parameter_overrides().updValue(idx).set_parameter_value(newValue);
        }

        const OpenSim::Component& impl_get_component() const final
        {
            return *this;
        }

        bool impl_can_upd_component() const final
        {
            return true;
        }

        OpenSim::Component& impl_upd_component() final
        {
            throw std::runtime_error{ "component updating not implemented for this IComponentAccessor" };
        }
    };
}
