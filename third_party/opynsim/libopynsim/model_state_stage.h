#pragma once

namespace opyn
{
    // Represents a stage of state realization. A `ModelState` advances
    // through each `ModelStateStage` in sequential order until it is
    // fully realized.
    //
    // Related: https://simtk.org/api_docs/molmodel/api_docs22/Simbody/html/classSimTK_1_1Stage.html
    // Related: https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089017/SimTK+Simulation+Concepts
    enum class ModelStateStage {

        // Time has advanced and state variables have new values, but no derived information has been calculated.
        time = 0,

        // The spatial positions of all bodies are known.
        position,

        // The spatial velocities of all bodies are known.
        velocity,

        // The force acting on each body is known, along with total kinetic/potential energy.
        dynamics,

        // The time derivatives of all continuous state variables known.
        acceleration,

        // Additional variables useful for output are known (optional: not required for integration).
        report,

        NUM_OPTIONS
    };
}
