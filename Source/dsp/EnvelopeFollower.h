#pragma once
#include <juce_dsp/juce_dsp.h>

// 全波整流 + 1次 LP による振幅エンベロープ抽出
// Attack: 上昇時の時定数  Release: 下降時の時定数
class EnvelopeFollower
{
public:
    void prepare(double sampleRate);
    void setAttackMs(float ms);
    void setReleaseMs(float ms);

    float process(float sample) noexcept;
    void  reset() { envelope = 0.0f; }

private:
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope     = 0.0f;
    double sampleRate  = 44100.0;

    static float msToCoeff(float ms, double sr) noexcept;
};
