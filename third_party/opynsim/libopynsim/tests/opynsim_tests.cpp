#include <gtest/gtest.h>

#include <vector>

int main(int argc, char** argv)
{
    // This little dance can be handy for modifying what `opynsim_tests` does when
    // ran from an IDE where editing the run configuration is annoying to do.
    std::vector<std::string> extra_args = {
        // "--gtest_filter=UndoableModelStatePair.CanLoadModelWithMuscleEquilibrationProblems",  // e.g.
    };
    std::vector<char*> forwarded_args;
    forwarded_args.reserve(static_cast<size_t>(argc) + extra_args.size());
    forwarded_args.insert(forwarded_args.end(), argv, argv + argc);
    for (auto& extra_arg : extra_args) {
        forwarded_args.push_back(extra_arg.data());
    }
    auto size = static_cast<int>(forwarded_args.size());

    testing::InitGoogleTest(&size, forwarded_args.data());
    return RUN_ALL_TESTS();
}
