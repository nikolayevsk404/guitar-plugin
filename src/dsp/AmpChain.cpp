#include "AmpChain.h"

void AmpChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    tubeScreamer.prepare (spec);
    preamp.prepare (spec);
    toneStack.prepare (spec);
    powerAmp.prepare (spec);
    namAmp.prepare (spec);
    cabinet.prepare (spec);
    ambience.prepare (spec);
    masterGain.reset (spec.sampleRate, 0.02);
    gateEnvelope = 0.0f;
}

void AmpChain::reset()
{
    tubeScreamer.reset();
    preamp.reset();
    toneStack.reset();
    powerAmp.reset();
    namAmp.reset();
    cabinet.reset();
    ambience.reset();
    gateEnvelope = 0.0f;
}

void AmpChain::setMode (AmpMode mode)
{
    currentMode = mode;
    preamp.setMode (mode);
    powerAmp.setMode (mode);
    ambience.setChorusEnabled (mode == AmpMode::Clean);
}

void AmpChain::setAmpGain (float value01) { preamp.setGain (value01); }
void AmpChain::setBass (float value01) { toneStack.setBass (value01); }
void AmpChain::setMid (float value01) { toneStack.setMid (value01); }
void AmpChain::setTreble (float value01) { toneStack.setTreble (value01); }
void AmpChain::setPresence (float value01) { toneStack.setPresence (value01); }
void AmpChain::setMaster (float value01) { masterGain.setTargetValue (value01); }

void AmpChain::setTsDrive (float value01) { tubeScreamer.setDrive (value01); }
void AmpChain::setTsTone (float value01) { tubeScreamer.setTone (value01); }
void AmpChain::setTsLevel (float value01) { tubeScreamer.setLevel (value01); }
void AmpChain::setTsBypass (bool bypass) { tubeScreamer.setBypass (bypass); }

bool AmpChain::loadIrFromFile (const juce::File& file) { return cabinet.loadIrFromFile (file); }
void AmpChain::loadDefaultIr() { cabinet.loadDefaultIr(); }

void AmpChain::applyNoiseGate (juce::dsp::AudioBlock<float>& block)
{
    const auto numSamples = block.getNumSamples();
    const auto numChannels = block.getNumChannels();

    for (size_t i = 0; i < numSamples; ++i)
    {
        float peak = 0.0f;
        for (size_t ch = 0; ch < numChannels; ++ch)
            peak = juce::jmax (peak, std::abs (block.getChannelPointer (ch)[i]));

        const auto targetEnv = peak > noiseGateThreshold ? 1.0f : 0.0f;
        const auto coeff = targetEnv > gateEnvelope ? gateAttackCoeff : gateReleaseCoeff;
        gateEnvelope += coeff * (targetEnv - gateEnvelope);

        for (size_t ch = 0; ch < numChannels; ++ch)
            block.getChannelPointer (ch)[i] *= gateEnvelope;
    }
}

void AmpChain::process (juce::dsp::AudioBlock<float>& block)
{
    applyNoiseGate (block);
    tubeScreamer.process (block);

    if (currentMode == AmpMode::Clean)
    {
        preamp.process (block);
        toneStack.process (block);
        powerAmp.process (block);
    }
    else
    {
        // Rhythm/Lead: cabecote real via captura Neural Amp Modeler (assets/nam/nikolayevsk.nam)
        // no lugar da cadeia analogica sintetica. O NAM ja captura o comportamento nao-linear
        // completo do cabecote (preamp + power amp), entao PowerAmpSaturation e pulado aqui.
        // ToneStack continua rodando depois, como EQ de shaping (Bass/Mid/Treble/Presence
        // seguem funcionais, so nao ficam mais "no meio" do amp, ja que o NAM e monolitico).
        namAmp.process (block);
        toneStack.process (block);
    }

    cabinet.process (block);

    // Ambiencia so em Clean (chorus+slapback+reverb) e Lead (so slapback+reverb, sem chorus --
    // ver AmpChain::setMode/AmbienceStage::setChorusEnabled). Rhythm fica seco e tight, sem
    // lavar o hi-gain (FR-003, fusao djent/prog/power metal pedida pelo usuario).
    if (currentMode == AmpMode::Clean || currentMode == AmpMode::Lead)
        ambience.process (block);

    const auto numSamples = block.getNumSamples();
    for (size_t i = 0; i < numSamples; ++i)
    {
        const auto gain = masterGain.getNextValue();
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            block.getChannelPointer (ch)[i] *= gain;
    }
}
