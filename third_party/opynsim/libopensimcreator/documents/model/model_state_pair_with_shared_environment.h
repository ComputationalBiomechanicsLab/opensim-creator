#pragma once

#include <libopynsim/documents/model/model_state_pair.h>

#include <memory>

namespace osc { class Environment; }

namespace osc
{
    class ModelStatePairWithSharedEnvironment : public opyn::ModelStatePair {
    public:
        std::shared_ptr<Environment> tryUpdEnvironment() const { return implUpdAssociatedEnvironment(); }
    private:
        virtual std::shared_ptr<Environment> implUpdAssociatedEnvironment() const { return nullptr; }
    };
}
