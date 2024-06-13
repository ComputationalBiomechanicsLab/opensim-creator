#include <oscar/Utils/FileChangePoller.h>

#include <gtest/gtest.h>

using namespace osc;

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(FileChangePoller, CtorDoesNotThrowExceptionIfGivenInvalidPath)
{
    const std::string path = "doesnt-exist";

    // constructing with an invalid path shouldn't throw
    ASSERT_NO_THROW({ FileChangePoller(std::chrono::milliseconds{0}, path); });
}

// repro for #495
//
// @JuliaVanBeesel reported that, when editing an OpenSim model via the editor UI, if
// they then delete the backing file (e.g. via Windows explorer), the editor UI will
// then show an error message from an exception, rather than carrying on or warning
// that something not-quite-right has happened
TEST(FileChangePoller, ChangeWasDetectedDoesNotThrowExceptionIfGivenInvalidPath)
{
    const std::string path = "doesnt-exist";

    // construct it with an invalid path
    FileChangePoller p{std::chrono::milliseconds{0}, path};

    // change_detected should return `false` (as in, no change detected) if the file
    // does not exist (e.g. because it was deleted by a user)
    //
    // (maybe this method should return an enum { NoChange, Change, Missing } )
    ASSERT_FALSE(p.change_detected(path));
}
