#include "AmbienceStage.h"

void AmbienceStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    chorus.prepare (spec);
    chorus.setRate (0.8f);
    chorus.setDepth (0.18f);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.08f);
    chorus.setMix (0.18f);

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples (static_cast<int> (spec.sampleRate * 0.5));
    delayLine.setDelay (static_cast<float> (spec.sampleRate * 0.11)); // ~110ms, slapback

    juce::Reverb::Parameters params;
    params.roomSize = 0.32f;
    params.damping = 0.5f;
    params.wetLevel = 0.24f;
    params.dryLevel = 0.85f;
    params.width = 0.9f;
    reverb.setParameters (params);
    reverb.setSampleRate (spec.sampleRate);
}

void AmbienceStage::reset()
{
    chorus.reset();
    delayLine.reset();
    reverb.reset();
}

void AmbienceStage::process (juce::dsp::AudioBlock<float>& block)
{
    if (chorusEnabled)
    {
        juce::dsp::ProcessContextReplacing<float> chorusContext (block);
        chorus.process (chorusContext);
    }

    const auto numSamples = block.getNumSamples();
    const auto numChannels = block.getNumChannels();

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto* data = block.getChannelPointer (ch);
        const auto channelIndex = static_cast<int> (ch);

        for (size_t i = 0; i < numSamples; ++i)
        {
            const auto delayed = delayLine.popSample (channelIndex);
            delayLine.pushSample (channelIndex, data[i] + delayed * delayFeedback);
            data[i] += delayed * delayMix;
        }
    }

    if (numChannels >= 2)
        reverb.processStereo (block.getChannelPointer (0), block.getChannelPointer (1), static_cast<int> (numSamples));
    else if (numChannels == 1)
        reverb.processMono (block.getChannelPointer (0), static_cast<int> (numSamples));
}
