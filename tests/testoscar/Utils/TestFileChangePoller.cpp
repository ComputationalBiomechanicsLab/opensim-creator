#include <oscar/Utils/FileChangePoller.h>

#include <gtest/gtest.h>

#include <chrono>
#include <string>

using namespace osc;
using namespace std::chrono_literals;

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(FileChangePoller, constructor_does_not_throw_exception_when_given_invalid_path)
{
    const std::string path = "doesnt-exist";

    // constructing with an invalid path shouldn't throw
    ASSERT_NO_THROW({ FileChangePoller(0ms, path); });
}

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(FileChangePoller, change_detected_does_not_throw_exception_if_given_invalid_path)
{
    const std::string path = "doesnt-exist";

    // construct it with an invalid path
    FileChangePoller p{0ms, path};

    // change_detected should return `false` (as in, no change detected) if the file
    // does not exist (e.g. because it was deleted by a user)
    ASSERT_FALSE(p.change_detected(path));
}
