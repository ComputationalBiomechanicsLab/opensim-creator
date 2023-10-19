#include <gtest/gtest.h>

TEST(MetaSuite1, Test1)
{
    // this is here to ensure that the test actually turns up in the output/IDE
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
