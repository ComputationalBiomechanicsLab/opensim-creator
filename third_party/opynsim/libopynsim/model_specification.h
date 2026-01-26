#pragma once

#include <libopynsim/model.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>

#include <filesystem>

namespace OpenSim { class Model; }

namespace opyn
{
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
