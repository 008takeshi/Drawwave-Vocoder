#include "UnvoicedDetector.h"
#include <cmath>

void UnvoicedDetector::prepare(double) noexcept {}

bool UnvoicedDetector::process(const float* signal, int numSamples) noexcept
{
    if (numSamples <= 0) return true;

    int   zcr    = 0;
    float rmsSum = 0.0f;
    float prev   = signal[0];

    for (int s = 1; s < numSamples; ++s)
    {
        if ((prev < 0.0f) != (signal[s] < 0.0f)) ++zcr;
        rmsSum += signal[s] * signal[s];
        prev = signal[s];
    }

    const float zcrNorm  = (float)zcr / (float)numSamples;
    const float rms      = std::sqrt(rmsSum / (float)numSamples);
    const float threshold = 0.05f + sensitivity * 0.45f;  // [0.05, 0.5]

    return (zcrNorm < threshold) && (rms > 0.001f);
}
