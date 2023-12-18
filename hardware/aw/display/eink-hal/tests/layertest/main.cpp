
#include <gtest/gtest.h>

extern void LayerPlannerTestInit();

int main(int argc, char *argv[])
{
    LayerPlannerTestInit();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
