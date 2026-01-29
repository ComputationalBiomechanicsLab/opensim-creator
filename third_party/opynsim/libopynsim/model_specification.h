#pragma once

#include <libopynsim/model.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <filesystem>

namespace OpenSim { class Model; }

namespace opyn
{
    // Represents a high-level model specification that can be validated
    // and compiled into a `Model`.
    //
    // Related: https://simtk.org/api_docs/opensim/api_docs32/classOpenSim_1_1Model.html#details
    // Related: https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089017/SimTK+Simulation+Concepts
    class ModelSpecification {
    public:
        static ModelSpecification from_osim_file(const std::filesystem::path&);

    private:
        class Impl;
        explicit ModelSpecification(osc::CopyOnUpdPtr<Impl>);

    public:
        Model compile() const;

    private:
        osc::CopyOnUpdPtr<Impl> impl_;
    };
}
