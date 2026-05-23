#include "EnvelopeFollower.h"
#include <cmath>

float EnvelopeFollower::msToCoeff(float ms, double sr) noexcept
{
    if (ms <= 0.0f) return 1.0f;
    return 1.0f - std::exp(-1.0f / (float)(sr * ms * 0.001));
}

void EnvelopeFollower::prepare(double sr)
{
    sampleRate = sr;
    setAttackMs(5.0f);
    setReleaseMs(50.0f);
    reset();
}

void EnvelopeFollower::setAttackMs(float ms)
{
    attackCoeff = msToCoeff(ms, sampleRate);
}

void EnvelopeFollower::setReleaseMs(float ms)
{
    releaseCoeff = msToCoeff(ms, sampleRate);
}

float EnvelopeFollower::process(float sample) noexcept
{
    const float rect = std::abs(sample);
    const float coeff = (rect > envelope) ? attackCoeff : releaseCoeff;
    envelope += coeff * (rect - envelope);
    return envelope;
}
