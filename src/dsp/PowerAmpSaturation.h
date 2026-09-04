#pragma once

#include <juce_dsp/juce_dsp.h>
#include "../plugin/ParameterLayout.h"

// Segundo estagio de saturacao, mais sutil, simulando "power tube sag" (plan.md, secao
// DSP Chain item 6). Sem parametro de UI dedicado por ora -- intensidade e fixa por modo.
class PowerAmpSaturation
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setMode (AmpMode newMode);

private:
    AmpMode mode = AmpMode::Clean;
};
