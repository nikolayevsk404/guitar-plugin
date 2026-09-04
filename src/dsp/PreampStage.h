#pragma once

#include <juce_dsp/juce_dsp.h>
#include "../plugin/ParameterLayout.h"

// Estagio de preamp dependente de modo (FR-002/FR-003). Os dois voicings de alto gain usam
// identificadores internos BRIT_HI/CALI_HI apenas em comentario/racional -- nunca expostos
// como string em UI, preset ou metadado (guardrail de trademark, ver plan.md).
// Curvas/numeros de estagio ainda nao calibrados via A/B de escuta (T012/T052 em tasks.md).
class PreampStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setMode (AmpMode newMode);
    void setGain (float gain01);

private:
    void updateFilters();

    AmpMode mode = AmpMode::Clean;
    juce::SmoothedValue<float> gainAmount { 0.4f };
    double sampleRate = 44100.0;

    // Passa-alta pre-gain: mais agressivo em Rhythm/Lead para controlar graves (voicing
    // "CALI_HI" -- research.md secao 1). Troca de coeficiente ao mudar de modo ainda pode
    // gerar transiente audivel; suavizacao completa fica para T016.
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> preFilter;

    // Passa-alta POS-clipping, so em Rhythm/Lead ("tightening" -- tecnica comum em amps de
    // alto gain djent/prog: o proprio clipping acrescenta harmonicos graves que engordam e
    // amolecem o palm mute; um corte depois do clip resolve sem perder o corpo do sinal
    // original, que ja passou pelo preFilter antes do clip).
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> tightenFilter;
};
