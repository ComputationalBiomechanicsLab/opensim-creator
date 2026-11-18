#pragma once

#include <libopynsim/documents/model/model_state_pair.h>

#include <memory>

namespace osc
{
    class Environment;

    class ModelStatePairWithSharedEnvironment : public opyn::ModelStatePair {
    public:
        std::shared_ptr<Environment> try_upd_environment() const
        {
            return impl_upd_associated_environment();
        }

    private:
        virtual std::shared_ptr<Environment> impl_upd_associated_environment() const
        {
            return nullptr;
        }
    };
}
