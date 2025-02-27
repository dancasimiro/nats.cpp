#include <gtest/gtest.h>
#include "../include/repl.h"

class ReplTest : public ::testing::Test {
protected:
    //REPL repl;

    void SetUp() override {
        // Any necessary setup before each test
    }

    void TearDown() override {
        // Any necessary cleanup after each test
    }
};

TEST_F(ReplTest, PrintResult) {
    // std::ostringstream output;
    // const std::string result = "5";
    // repl.printResult(output, result);
    // EXPECT_EQ(output.str(), "Result: 5\n");
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
