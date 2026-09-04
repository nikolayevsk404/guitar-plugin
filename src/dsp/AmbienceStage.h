#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

// Chorus + slapback delay + reverb curta, leves, fixos (sem parametro de UI por ora) --
// usado no Clean (chorus+delay+reverb) e no Lead (so delay+reverb, ver setChorusEnabled).
// AmpChain.cpp so chama process() nesses dois modos, pra nao gastar CPU nem colorir Rhythm.
class AmbienceStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    // Clean usa chorus+delay+reverb; Lead usa so delay+reverb (chorus desligado).
    void setChorusEnabled (bool shouldBeEnabled) { chorusEnabled = shouldBeEnabled; }

private:
    bool chorusEnabled = true;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float> delayLine { 96000 };
    static constexpr float delayFeedback = 0.2f;
    static constexpr float delayMix = 0.2f;

    juce::Reverb reverb;
};
