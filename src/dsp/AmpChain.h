#pragma once

#include "TubeScreamerStage.h"
#include "PreampStage.h"
#include "ToneStack.h"
#include "PowerAmpSaturation.h"
#include "NamAmpStage.h"
#include "CabinetLoader.h"
#include "AmbienceStage.h"

// Orquestra a cadeia completa (plan.md, "DSP Chain"):
// NoiseGate -> TubeScreamerStage ->
//   Clean: PreampStage -> ToneStack -> PowerAmpSaturation
//   Rhythm/Lead: NamAmpStage (captura real do cabecote) -> ToneStack
// -> CabinetLoader -> [AmbienceStage, Clean com chorus / Lead so delay+reverb] ->
// Master (output trim).
class AmpChain
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setMode (AmpMode mode);
    void setAmpGain (float value01);
    void setBass (float value01);
    void setMid (float value01);
    void setTreble (float value01);
    void setPresence (float value01);
    void setMaster (float value01);

    void setTsDrive (float value01);
    void setTsTone (float value01);
    void setTsLevel (float value01);
    void setTsBypass (bool bypass);

    bool loadIrFromFile (const juce::File& file);
    void loadDefaultIr();
    bool isUsingDefaultIr() const { return cabinet.isUsingDefaultIr(); }
    juce::String getCurrentIrName() const { return cabinet.getCurrentIrName(); }

private:
    void applyNoiseGate (juce::dsp::AudioBlock<float>& block);

    TubeScreamerStage tubeScreamer;
    PreampStage preamp;
    ToneStack toneStack;
    PowerAmpSaturation powerAmp;
    NamAmpStage namAmp;
    CabinetLoader cabinet;
    AmbienceStage ambience;

    AmpMode currentMode = AmpMode::Clean;
    juce::SmoothedValue<float> masterGain { 0.7f };

    // Gate fixo por ora (nao exposto como parametro de UI ainda) -- necessario porque
    // Rhythm/Lead tem ganho alto o suficiente para expor chiado de fundo (plan.md, item 2
    // da DSP Chain). Threshold em amplitude linear, ~-50dBFS.
    static constexpr float noiseGateThreshold = 0.003f;
    static constexpr float gateAttackCoeff = 0.6f;
    static constexpr float gateReleaseCoeff = 0.05f;
    float gateEnvelope = 0.0f;
};
