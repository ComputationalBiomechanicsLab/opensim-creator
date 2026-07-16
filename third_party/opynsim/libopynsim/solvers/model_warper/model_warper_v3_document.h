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
        ModelWarperV3Document()
        {
            constructProperty_scaling_parameter_overrides();
        }

        bool hasScalingSteps() const
        {
            if (getNumImmediateSubcomponents() == 0) {
                return false;
            }
            const auto lst = getComponentList<ScalingStep>();
            return lst.begin() != lst.end();
        }

        auto iterateScalingSteps() const
        {
            return getComponentList<ScalingStep>();
        }

        void addScalingStep(std::unique_ptr<ScalingStep> step)
        {
            addComponent(step.release());
            clearConnections();
            finalizeConnections(*this);
            finalizeFromProperties();
        }

        bool removeScalingStep(ScalingStep& step)
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

        bool hasScalingParameters() const
        {
            if (not hasScalingSteps()) {
                return false;
            }
            for (const ScalingStep& step : iterateScalingSteps()) {
                bool called = false;
                step.forEachScalingParameterDeclaration([&called](const ScalingParameterDeclaration&) { called = true; });
                if (called) {
                    return true;
                }
            }
            return false;
        }

        ScalingParameters getEffectiveScalingParameters() const
        {
            ScalingParameters rv;

            // Get/merge values from the scaling steps
            for (const ScalingStep& step : iterateScalingSteps()) {
                step.forEachScalingParameterDeclaration([&step, &rv](const ScalingParameterDeclaration& decl)
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

        bool setScalingParameterOverride(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            mutateScalingParammeterOverridesWithNewOverride(scalingParamName, newValue);
            finalizeFromProperties();
            return true;
        }

        void saveTo(const std::filesystem::path& p) const
        {
            print(p.string());
        }

    private:
        void mutateScalingParammeterOverridesWithNewOverride(const std::string& scalingParamName, ScalingParameterValue newValue)
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

        const OpenSim::Component& implGetComponent() const final
        {
            return *this;
        }

        bool implCanUpdComponent() const final
        {
            return true;
        }

        OpenSim::Component& implUpdComponent() final
        {
            throw std::runtime_error{ "component updating not implemented for this IComponentAccessor" };
        }
    };
}
