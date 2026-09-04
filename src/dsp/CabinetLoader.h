#pragma once

#include <juce_dsp/juce_dsp.h>

// Caixa + microfone via convolucao de IR (FR-005/FR-006). Carrega o IR embutido por padrao
// (plug-and-play, FR-009) e permite trocar por um IR externo do usuario.
//
// ATENCAO: a assinatura exata de juce::dsp::Convolution::loadImpulseResponse (overloads para
// bloco de memoria vs juce::File, e os enums Stereo/Trim/Normalise) variou entre versoes do
// JUCE -- validar contra a versao vendorizada via FetchContent (ver CMakeLists.txt) antes de
// considerar esta classe "pronta" (T014 em tasks.md).
class CabinetLoader
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    // Retorna false (e mantem o IR anterior carregado) se o arquivo for invalido ou
    // incompativel -- edge case da spec: nunca deixar o plugin "mudo" por IR ruim.
    bool loadIrFromFile (const juce::File& file);
    void loadDefaultIr();

    bool isUsingDefaultIr() const { return usingDefaultIr; }
    juce::String getCurrentIrName() const { return currentIrName; }

private:
    juce::dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 512 } };
    juce::dsp::ProcessSpec currentSpec { 44100.0, 512, 2 };
    bool usingDefaultIr = true;
    juce::String currentIrName = "nikolayevsk";
};
