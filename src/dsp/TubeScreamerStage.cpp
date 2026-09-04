#include "TubeScreamerStage.h"

void TubeScreamerStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    driveGain.reset (spec.sampleRate, 0.02);
    outputGain.reset (spec.sampleRate, 0.02);
    toneFilter.prepare (spec);
    updateToneFilter();
}

void TubeScreamerStage::reset()
{
    toneFilter.reset();
}

void TubeScreamerStage::setDrive (float drive01)
{
    // 0..1 mapeado para ganho de pre-clipping de ~1x a ~25x (research.md secao 2).
    driveGain.setTargetValue (juce::jmap (drive01, 0.0f, 1.0f, 1.0f, 25.0f));
}

void TubeScreamerStage::setTone (float tone01)
{
    toneNormalised = tone01;
    updateToneFilter();
}

void TubeScreamerStage::setLevel (float level01)
{
    outputGain.setTargetValue (level01);
}

void TubeScreamerStage::setBypass (bool shouldBypass)
{
    bypassed = shouldBypass;
}

void TubeScreamerStage::updateToneFilter()
{
    // Shelving de agudos pos-clipping: tone01=0 escurece, tone01=1 realca.
    const auto gainDb = juce::jmap (toneNormalised, 0.0f, 1.0f, -6.0f, 6.0f);
    *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 1800.0, 0.7f, juce::Decibels::decibelsToGain (gainDb));
}

void TubeScreamerStage::process (juce::dsp::AudioBlock<float>& block)
{
    if (bypassed)
        return; // hard bypass -- sinal bit-perfeito, conforme research.md secao 2

    const auto numSamples = block.getNumSamples();
    const auto numChannels = block.getNumChannels();

    for (size_t i = 0; i < numSamples; ++i)
    {
        const auto drive = driveGain.getNextValue();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer (ch);
            const auto driven = data[i] * drive;
            // Curva assimetrica leve: diodos reais nao clippam simetricamente.
            data[i] = driven >= 0.0f ? std::tanh (driven) : 0.9f * std::tanh (driven);
        }
    }

    juce::dsp::ProcessContextReplacing<float> context (block);
    toneFilter.process (context);

    for (size_t i = 0; i < numSamples; ++i)
    {
        const auto level = outputGain.getNextValue();

        for (size_t ch = 0; ch < numChannels; ++ch)
            block.getChannelPointer (ch)[i] *= level;
    }
}
