#include "CabinetLoader.h"
#include "BinaryData.h"
#include "../util/DiagnosticLog.h"
#include <juce_audio_formats/juce_audio_formats.h>

void CabinetLoader::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSpec = spec;
    convolution.prepare (spec);

    // Sanity check do embed -- se o BinaryData::nikolayevsk_wav viesse vazio/corrompido (ex.:
    // problema de build especifico de plataforma), e melhor saber disso aqui do que so notar
    // "IR nao pega o som direito" depois.
    logDiagnosticEvent ("IR", "prepare -- BinaryData nikolayevsk_wavSize=" + juce::String (BinaryData::nikolayevsk_wavSize)
                                  + " sampleRate=" + juce::String (spec.sampleRate)
                                  + " numChannels=" + juce::String (spec.numChannels));

    loadDefaultIr();
}

void CabinetLoader::reset()
{
    convolution.reset();
}

void CabinetLoader::loadDefaultIr()
{
    convolution.loadImpulseResponse (
        BinaryData::nikolayevsk_wav,
        static_cast<size_t> (BinaryData::nikolayevsk_wavSize),
        juce::dsp::Convolution::Stereo::no,
        juce::dsp::Convolution::Trim::yes,
        0,
        juce::dsp::Convolution::Normalise::yes);

    usingDefaultIr = true;
    currentIrName = "nikolayevsk";

    // juce::dsp::Convolution::loadImpulseResponse nao retorna sucesso/falha -- entao "logamos
    // que tentamos" e nao "logamos que deu certo". Se nikolayevsk_wavSize (acima) estiver
    // plausivel (dezenas/centenas de KB) e mesmo assim o cabinet nao colorir o som, o suspeito
    // deixa de ser o carregamento e passa a ser o proprio processamento da convolucao.
    logDiagnosticEvent ("IR", "loadDefaultIr -- convolution.loadImpulseResponse chamado (nikolayevsk)");
}

bool CabinetLoader::loadIrFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
    {
        logDiagnosticEvent ("IR", "loadIrFromFile -- arquivo nao existe: " + file.getFullPathName());
        return false;
    }

    // juce::dsp::Convolution nao expoe um retorno de sucesso direto em todas as versoes --
    // validamos o arquivo com um AudioFormatManager antes de tentar carregar, para poder
    // aplicar o fallback do edge case da spec (IR invalido -> mantem o IR anterior).
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr || reader->numChannels == 0 || reader->sampleRate <= 0.0)
    {
        logDiagnosticEvent ("IR", "loadIrFromFile -- AudioFormatManager rejeitou o arquivo (reader nulo ou invalido): "
                                      + file.getFullPathName());
        return false;
    }

    convolution.loadImpulseResponse (
        file,
        juce::dsp::Convolution::Stereo::no,
        juce::dsp::Convolution::Trim::yes,
        0,
        juce::dsp::Convolution::Normalise::yes);

    usingDefaultIr = false;
    currentIrName = file.getFileName();

    logDiagnosticEvent ("IR", "loadIrFromFile -- ok, " + juce::String (reader->numChannels) + "ch @ "
                                  + juce::String (reader->sampleRate) + "Hz: " + file.getFullPathName());
    return true;
}

void CabinetLoader::process (juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> context (block);
    convolution.process (context);
}
