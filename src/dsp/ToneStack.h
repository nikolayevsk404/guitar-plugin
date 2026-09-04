#pragma once

#include <juce_dsp/juce_dsp.h>

// Bass/Mid/Treble/Presence (FR-007). Frequencias de corte sao ponto de partida heuristico,
// nao calibradas por escuta ainda (T011/T052 em tasks.md).
class ToneStack
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setBass (float value01);
    void setMid (float value01);
    void setTreble (float value01);
    void setPresence (float value01);

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;

    double sampleRate = 44100.0;
    float bass01 = 0.5f, mid01 = 0.5f, treble01 = 0.5f, presence01 = 0.5f;

    Filter bassFilter;     // low shelf ~100Hz
    Filter midFilter;      // peak ~800Hz (permite mid scoop em Rhythm/Lead, ver research.md)
    Filter trebleFilter;   // high shelf ~3kHz
    Filter presenceFilter; // high shelf ~6kHz, mais estreito/agudo que o treble
};
