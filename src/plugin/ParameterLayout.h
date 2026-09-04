#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Contrato unico de parametros entre C++ (APVTS) e a UI React/TS
// (ui/src/bridge/nativeBridge.ts). Os IDs abaixo DEVEM ser mantidos em sincronia manualmente
// com PluginParams em nativeBridge.ts -- nao ha checagem automatica entre os dois lados ainda
// (ver Open Question / risco em specs/001-nikolayevsk-dark-dsp/research.md).

namespace ParamId
{
    static constexpr auto ampMode = "ampMode";
    static constexpr auto gain = "gain";
    static constexpr auto bass = "bass";
    static constexpr auto mid = "mid";
    static constexpr auto treble = "treble";
    static constexpr auto presence = "presence";
    static constexpr auto master = "master";
    static constexpr auto tsDrive = "tsDrive";
    static constexpr auto tsTone = "tsTone";
    static constexpr auto tsLevel = "tsLevel";
    static constexpr auto tsBypass = "tsBypass";
}

// Ordem deve bater com o union type AmpMode ("clean" | "rhythm" | "lead") em nativeBridge.ts.
// Nomenclatura interna de voicing (BRIT_HI/CALI_HI, ver plan.md) fica em PreampStage, nunca
// exposta aqui como choice de UI.
enum class AmpMode
{
    Clean = 0,
    Rhythm = 1,
    Lead = 2
};

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamId::ampMode, 1 }, "Amp Mode",
        StringArray { "Clean", "Rhythm", "Lead" }, static_cast<int>(AmpMode::Clean)));

    auto addUnitFloat = [&params](const char* id, const char* name, float defaultValue)
    {
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float> { 0.0f, 1.0f }, defaultValue));
    };

    addUnitFloat(ParamId::gain, "Gain", 0.4f);
    addUnitFloat(ParamId::bass, "Bass", 0.5f);
    addUnitFloat(ParamId::mid, "Mid", 0.5f);
    addUnitFloat(ParamId::treble, "Treble", 0.5f);
    addUnitFloat(ParamId::presence, "Presence", 0.5f);
    addUnitFloat(ParamId::master, "Master", 0.7f);

    addUnitFloat(ParamId::tsDrive, "TS Drive", 0.3f);
    addUnitFloat(ParamId::tsTone, "TS Tone", 0.5f);
    addUnitFloat(ParamId::tsLevel, "TS Level", 0.7f);

    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { ParamId::tsBypass, 1 }, "TS Bypass", true));

    return { params.begin(), params.end() };
}
