#include <gtest/gtest.h>
#include "../jell_fft.hpp"
#include "../AudioFrame.hpp"


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

TEST(BasicTest, Rms)
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

    AudioFrame frame = AudioFrame(samples, 8, 100, 64000, 90000, 0, 0);

    EXPECT_NEAR(frame.rms.value, 744.932556, 0.01);
    EXPECT_NEAR(frame.rms.smoothed_level, 0.05, 0.01);

    EXPECT_EQ(frame.rms.max, 90000);

}

TEST(BasicTest, RmsMax)
{
    int32_t samples[] = {
        10000,
        999999999,
       110700,
       110000,
        50700,
        10000,
       -30700,
       -90000,
       -90700
   };

    AudioFrame frame = AudioFrame(samples, 8, 10000, 64000, 90000, 0, 0);

    EXPECT_EQ(frame.rms.max, 353553408);
}