#pragma once

#include <juce_dsp/juce_dsp.h>

// Pedal overdrive estilo Tube Screamer (FR-004). Racional da curva em
// specs/001-nikolayevsk-dark-dsp/research.md secao 2. Implementacao ainda nao passou pelo
// ciclo test-first da Fase 1 de tasks.md (T010) -- valores/curva sujeitos a calibracao.
class TubeScreamerStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setDrive (float drive01);
    void setTone (float tone01);
    void setLevel (float level01);
    void setBypass (bool shouldBypass);

private:
    void updateToneFilter();

    juce::SmoothedValue<float> driveGain { 1.0f };
    juce::SmoothedValue<float> outputGain { 1.0f };
    float toneNormalised = 0.5f;
    bool bypassed = true;
    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> toneFilter;
};
