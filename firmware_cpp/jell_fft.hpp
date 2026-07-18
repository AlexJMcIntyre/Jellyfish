//
// Created by Ryan Bibby on 17/07/2026.
//

#ifndef JELLYOS_JELL_FFT_HPP
#define JELLYOS_JELL_FFT_HPP
#include <cstdint>

#include "simple_fft/fft.h"

class JellFFT
{
    std::vector<std::complex<double>> data;

public:
    JellFFT(const int32_t* samples, const int sampleSize)
    {
        using Complex = std::complex<double>;
        std::vector<Complex> fft_data(sampleSize);

        for (int i = 0; i < sampleSize; ++i)
        {
            fft_data[i] = Complex(samples[i], 0.0);
        }
        this->data = fft_data;
    }

    std::vector<float> doIt()
    {
        const char* error = nullptr;
        simple_fft::FFT(data, data.size(), error);

        std::vector<float> magnitudes(data.size() / 2);

        for (std::size_t i = 0; i < data.size() / 2; ++i)
        {
            const double magnitude = std::abs(data[i]);
            printf("bin %zu: magnitude=%f\n", i, magnitude);
            magnitudes[i] = static_cast<float>(magnitude);
        }

        return magnitudes;
    }
};


#endif //JELLYOS_JELL_FFT_HPP
