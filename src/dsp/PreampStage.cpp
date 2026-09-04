#include "PreampStage.h"

void PreampStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    gainAmount.reset (spec.sampleRate, 0.02);
    preFilter.prepare (spec);
    tightenFilter.prepare (spec);
    updateFilters();
}

void PreampStage::reset()
{
    preFilter.reset();
    tightenFilter.reset();
}

void PreampStage::setMode (AmpMode newMode)
{
    mode = newMode;
    updateFilters();
}

void PreampStage::setGain (float gain01)
{
    gainAmount.setTargetValue (gain01);
}

void PreampStage::updateFilters()
{
    // Clean: headroom alto, corte suave. Rhythm/Lead: corte bem mais alto (voicing
    // "CALI_HI") pra manter graves "tight" em palm mute -- Rhythm mais apertado que Lead
    // (Lead mantem um pouco mais de corpo pro sustain), conforme research.md secao 1.
    const float preCutoffHz = mode == AmpMode::Clean ? 60.0f : (mode == AmpMode::Rhythm ? 140.0f : 100.0f);
    *preFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, preCutoffHz);

    const float tightenCutoffHz = mode == AmpMode::Rhythm ? 100.0f : 80.0f;
    *tightenFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, tightenCutoffHz);
}

void PreampStage::process (juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> preContext (block);
    preFilter.process (preContext);

    const auto numSamples = block.getNumSamples();
    const auto numChannels = block.getNumChannels();

    // Numero de estagios em cascata e multiplicador de drive por modo -- fusao de alto gain
    // britanico (presenca cortante, voicing "BRIT_HI") com ganho em cascata americano
    // (voicing "CALI_HI"), conforme tabela "Voicing Target" em spec.md. Retunado pra djent/
    // prog/power metal: Rhythm e Lead MUITO mais quentes que a calibracao original.
    const int cascadedStages = mode == AmpMode::Clean ? 1 : (mode == AmpMode::Rhythm ? 3 : 4);
    const float driveMultiplier = mode == AmpMode::Clean ? 3.0f : (mode == AmpMode::Rhythm ? 22.0f : 34.0f);

    for (size_t i = 0; i < numSamples; ++i)
    {
        const auto gain = gainAmount.getNextValue() * driveMultiplier;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer (ch);
            auto sample = data[i] * gain;

            for (int stage = 0; stage < cascadedStages; ++stage)
                sample = std::tanh (sample);

            data[i] = sample;
        }
    }

    if (mode != AmpMode::Clean)
    {
        juce::dsp::ProcessContextReplacing<float> tightenContext (block);
        tightenFilter.process (tightenContext);
    }
}
