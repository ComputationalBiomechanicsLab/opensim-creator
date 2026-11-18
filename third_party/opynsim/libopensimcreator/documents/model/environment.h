#pragma once

#include <libopensimcreator/documents/param_block.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>

#include <vector>

namespace osc
{
    // an environment that can be optionally associated with, multiple
    // `ModelStatePair`s (e.g. they all operate "in the same environment")
    class Environment final {
    public:
        Environment();

        const ParamBlock& get_simulation_params() const { return param_block_; }
        ParamBlock& upd_simulation_params() { return param_block_; }

        int get_num_user_output_extractors() const
        {
            return static_cast<int>(output_extractors_.size());
        }
        const opyn::SharedOutputExtractor& get_user_output_extractor(int index) const;
        void add_user_output_extractor(const opyn::SharedOutputExtractor& extractor);
        void remove_user_output_extractor(int index);
        bool has_user_output_extractor(const opyn::SharedOutputExtractor& extractor) const;
        bool remove_user_output_extractor(const opyn::SharedOutputExtractor& extractor);
        bool overwrite_or_add_new_user_output_extractor(
            const opyn::SharedOutputExtractor& old, const opyn::SharedOutputExtractor& newer);

        std::vector<opyn::SharedOutputExtractor> get_all_user_output_extractors() const;

    private:
        // simulation params: dictates how the next simulation shall be ran
        ParamBlock param_block_;

        // user-initiated output extractors
        //
        // simulators should try to hook into these, if the component exists
        std::vector<opyn::SharedOutputExtractor> output_extractors_;
    };
}
