#ifndef OPENSIM_MOCOEXPRESSIONBASEDPARAMETERGOAL_H
#define OPENSIM_MOCOEXPRESSIONBASEDPARAMETERGOAL_H
/* -------------------------------------------------------------------------- *
 * OpenSim: MocoExpressionBasedParameterGoal.h                                *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2024 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Allison John                                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "MocoGoal.h"
#include "OpenSim/Moco/MocoParameter.h"
#include <lepton/ExpressionProgram.h>
#include <SimTKcommon/internal/ReferencePtr.h>

namespace OpenSim {
class Model;

/** Minimize or constrain an arithmetic expression of parameters. 

This goal supports both "cost" and "endpoint constraint" modes and can be 
defined using any number of MocoParameters. The expression string should match 
the Lepton (lightweight expression parser) format.

# Creating Expressions

Expressions can be any string that represents a mathematical expression, e.g.,
"x*sqrt(y-8)". Expressions can contain variables, constants, operations,
parentheses, commas, spaces, and scientific "e" notation. The full list of
operations is: sqrt, exp, log, sin, cos, sec, csc, tan, cot, asin, acos, atan, 
sinh, cosh, tanh, erf, erfc, step, delta, square, cube, recip, min, max, abs, 
+, -, *, /, and ^.

# Examples

@code
auto* spring1_parameter = mp.addParameter("spring_stiffness", "spring1",
                                       "stiffness", MocoBounds(0, 100));
auto* spring2_parameter = mp.addParameter("spring2_stiffness", "spring2",
                                       "stiffness", MocoBounds(0, 100));
auto* spring_goal = mp.addGoal<MocoExpressionBasedParameterGoal>();
double STIFFNESS = 100.0;
// minimum is when p + q = STIFFNESS
spring_goal->setExpression(fmt::format("square(p+q-{})", STIFFNESS));
spring_goal->addParameter(*spring1_parameter, "p");
spring_goal->addParameter(*spring2_parameter, "q");
@endcode

@ingroup mocogoal */
class OSIMMOCO_API MocoExpressionBasedParameterGoal : public MocoGoal {
    OpenSim_DECLARE_CONCRETE_OBJECT(MocoExpressionBasedParameterGoal, MocoGoal);

public:
    MocoExpressionBasedParameterGoal() { constructProperties(); }
    MocoExpressionBasedParameterGoal(std::string name)
            : MocoGoal(std::move(name)) {
        constructProperties();
    }
    MocoExpressionBasedParameterGoal(std::string name, double weight)
            : MocoGoal(std::move(name), weight) {
        constructProperties();
    }
    MocoExpressionBasedParameterGoal(std::string name, double weight,
            std::string expression) : MocoGoal(std::move(name), weight) {
        constructProperties();
        set_expression(std::move(expression));
    }

    /** Set the arithmetic expression to minimize or constrain. Variable 
    names should match the names set with addParameter(). See "Creating 
    Expressions" in the class documentation above for an explanation of how to 
    create expressions. */
    void setExpression(std::string expression) {
        set_expression(std::move(expression));
    }

    /** Add parameters with variable names that match the variables in the
    expression string. All variables in the expression must have a corresponding
    parameter, but parameters with variables that are not in the expression are
    ignored. */
    void addParameter(const MocoParameter& parameter, std::string variable) {
        append_parameters(parameter);
        append_variables(std::move(variable));
    }

protected:
    void initializeOnModelImpl(const Model& model) const override;
    void calcGoalImpl(
            const GoalInput& input, SimTK::Vector& cost) const override;
    bool getSupportsEndpointConstraintImpl() const override { return true; }
    void printDescriptionImpl() const override;

private:
    void constructProperties();

    /** Get the value of the property from its index in the property_refs vector.
    This will use m_data_types to get the type, and if it is a Vec type, it uses
    m_indices to get the element to return, both at the same index i. */
    double getPropertyValue(int i) const;

    OpenSim_DECLARE_PROPERTY(expression, std::string,
            "The expression string defining this cost or endpoint constraint.");
    OpenSim_DECLARE_LIST_PROPERTY(parameters, MocoParameter,
            "Parameters included in the expression.");
    OpenSim_DECLARE_LIST_PROPERTY(variables, std::string,
            "Variables names corresponding to parameters in the expression.");

    mutable Lepton::ExpressionProgram m_program;
    // stores references to one property per parameter
    mutable std::vector<SimTK::ReferencePtr<const AbstractProperty>> m_property_refs;
    enum DataType {
      Type_double,
      Type_Vec3,
      Type_Vec6
    };
    mutable std::vector<DataType> m_data_types;
    mutable std::vector<int> m_indices;

};

} // namespace OpenSim

#endif // OPENSIM_MOCOPARAMETEREXPRESSIONGOAL_H
