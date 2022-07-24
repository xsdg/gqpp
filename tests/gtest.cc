#include "gtest/gtest.h"

#include <string.h>

//#include "main.h"
//#include "ifiledata.h"

TEST(FooTestClass, BarTest) {
    ASSERT_EQ(0, 0);
//    FileData::Util util;
}

int main (int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
