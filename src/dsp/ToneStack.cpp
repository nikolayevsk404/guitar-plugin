#include "ToneStack.h"

void ToneStack::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    bassFilter.prepare (spec);
    midFilter.prepare (spec);
    trebleFilter.prepare (spec);
    presenceFilter.prepare (spec);

    setBass (bass01);
    setMid (mid01);
    setTreble (treble01);
    setPresence (presence01);
}

void ToneStack::reset()
{
    bassFilter.reset();
    midFilter.reset();
    trebleFilter.reset();
    presenceFilter.reset();
}

void ToneStack::setBass (float value01)
{
    bass01 = value01;
    const auto gainDb = juce::jmap (value01, 0.0f, 1.0f, -12.0f, 12.0f);
    *bassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, 100.0, 0.7f, juce::Decibels::decibelsToGain (gainDb));
}

void ToneStack::setMid (float value01)
{
    mid01 = value01;
    const auto gainDb = juce::jmap (value01, 0.0f, 1.0f, -12.0f, 12.0f);
    *midFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, 800.0, 0.9f, juce::Decibels::decibelsToGain (gainDb));
}

void ToneStack::setTreble (float value01)
{
    treble01 = value01;
    const auto gainDb = juce::jmap (value01, 0.0f, 1.0f, -12.0f, 12.0f);
    *trebleFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 3000.0, 0.7f, juce::Decibels::decibelsToGain (gainDb));
}

void ToneStack::setPresence (float value01)
{
    presence01 = value01;
    const auto gainDb = juce::jmap (value01, 0.0f, 1.0f, -12.0f, 12.0f);
    *presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 6000.0, 1.0f, juce::Decibels::decibelsToGain (gainDb));
}

void ToneStack::process (juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> context (block);
    bassFilter.process (context);
    midFilter.process (context);
    trebleFilter.process (context);
    presenceFilter.process (context);
}
