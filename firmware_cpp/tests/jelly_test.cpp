#include <gtest/gtest.h>
#include "../jell_fft.hpp"

TEST(BasicTest, ConstantSignal)
{

    int32_t samples[] = {1, 1, 1, 1};
    JellFFT fft(samples, 4);
    std::vector<float> moo = fft.doIt();

    EXPECT_EQ(moo[0], 4);
}

TEST(BasicTest, SingleImpulse)
{

    int32_t samples[] = {1, 0, 0, 0};
    JellFFT fft(samples, 4);
    std::vector<float> moo = fft.doIt();

    EXPECT_EQ(moo[1], 1);
}

TEST(BasicTest, Signal)
{
    int32_t samples[] = {
        100,
       1107,
       1100,
        507,
        100,
       -307,
       -900,
       -907
   };
    JellFFT fft(samples, 8);
    std::vector<float> moo = fft.doIt();

    EXPECT_NEAR(moo[0], 800, 2.0);
    EXPECT_NEAR(moo[1], 4000, 2.0);
    EXPECT_NEAR(moo[2], 1200, 2.0);
    EXPECT_NEAR(moo[3], 0.3, 0.5);
}