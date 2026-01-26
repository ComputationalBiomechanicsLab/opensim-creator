#include "annotated_motion.h"

#include <libopynsim/tests/opynsim_tests_config.h>

#include <gtest/gtest.h>

#include <filesystem>

using namespace osc;

// repro for issue #1068
//
// User reported that providing a TRC file to OSC's "Preview Experimental Data" workflow
// causes it to crash, rather than emitting a warning or ignoring the column.
//
// The file loads fine in OpenSim GUI, which means OpenSim Creator must also support
// loading these kinds of files with similar fallback behavior to OpenSim GUI.
TEST(AnnotatedMotion, CanLoadTRCFileContainingSuperfluousMarkers)
{
    const std::filesystem::path reproFile =
        std::filesystem::path{OPYNSIM_TESTS_RESOURCES_DIR} / "opensim-creator_1068_repro.trc";

    const AnnotatedMotion motion(reproFile);  // shouldn't throw
    ASSERT_EQ(motion.getNumDataSeries(), 60) << "if this is 63, then maybe you have a problem - or coerced the marker names :-)";
}
