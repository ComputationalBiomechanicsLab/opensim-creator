#include <gtest/gtest.h>

#include <exception>

int main(int argc, char** argv)
{

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::GTEST_FLAG(catch_exceptions) = false;  // Ensure exceptions from `SetUpTestSuite` bubble up

    // Ensure any thrown exceptions cause the top-level process
    // to return `EXIT_FAILURE`: on Windows this isn't a guarantee O_o?
    int result = 0;
    try {
        result = RUN_ALL_TESTS();
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what();
        return EXIT_FAILURE;
    }

    // Both test failures and suite failures (e.g. when `SetUpTestSuite`
    // fails) should be treated as a failure.
    //
    // By default, `GTest::main` can let `SetUpTestSuite` failures through.
    const auto* unit = ::testing::UnitTest::GetInstance();
    if (unit->failed_test_count() > 0 ||
        unit->failed_test_suite_count() > 0) {

        return EXIT_FAILURE;
        }

    return result;
}
