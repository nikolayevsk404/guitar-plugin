#include "NamAmpStage.h"
#include "BinaryData.h"
#include "../util/DiagnosticLog.h"

#include <algorithm>

#include "dsp.h"
#include "get_dsp.h"
#include "slimmable.h"
#include "container.h"
#include "convnet.h"
#include "lstm.h"
#include "linear.h"
#include "wavenet/model.h"
#include "model_config.h"
#include "json.hpp"

namespace
{
// Toda arquitetura da NAM core lib (container.cpp, convnet.cpp, lstm.cpp, linear.cpp,
// wavenet/model.cpp) se auto-registra no ConfigParserRegistry via um objeto estatico global no
// fim do respectivo .cpp -- mas nada no resto do programa referencia simbolo nenhum desses
// arquivos diretamente (get_dsp() so consulta o registry em runtime pelo nome da arquitetura).
// Isso faz o linker podar o .o inteiro em QUALQUER estagio de link que va por biblioteca
// estatica (o NikolayevskDarkDsp "shared code" que o juce_add_plugin() gera e' STATIC, e liga
// assim no target por formato -- ver _juce_link_plugin_wrapper em JUCEUtils.cmake), mesmo com
// NamCore como OBJECT library (CMakeLists.txt) garantindo que o .o entre no primeiro link.
// Resultado: "No config parser registered for architecture: SlimmableContainer" (e teria sido
// "WaveNet" logo em seguida, ja que os submodelos de nikolayevsk.nam sao WaveNet) e
// createModel() sempre falha (carregados=0), plugin fica passthrough puro em Rhythm/Lead. Em
// vez de brigar com poda de linker em varios niveis, registramos explicito aqui -- este .cpp
// e' garantido no link porque NamAmpStage e' chamado direto por AmpChain. Registramos as 5
// (nao so SlimmableContainer+WaveNet, que sao as usadas hoje) pra essa classe de bug nao voltar
// se o .nam for trocado por um com outra arquitetura de submodelo.
void ensureNamParsersRegistered()
{
    auto& registry = nam::ConfigParserRegistry::instance();
    const auto registerIfMissing = [&registry] (const char* name, nam::ConfigParserFunction parser)
    {
        if (! registry.has (name))
            registry.registerParser (name, std::move (parser));
    };

    registerIfMissing ("SlimmableContainer", nam::container::create_config);
    registerIfMissing ("WaveNet", nam::wavenet::create_config);
    registerIfMissing ("ConvNet", nam::convnet::create_config);
    registerIfMissing ("LSTM", nam::lstm::create_config);
    registerIfMissing ("Linear", nam::linear::create_config);
}

std::unique_ptr<nam::DSP> createModel()
{
    ensureNamParsersRegistered();

    try
    {
        const auto json = nlohmann::json::parse (BinaryData::nikolayevsk_nam,
                                                   BinaryData::nikolayevsk_nam + BinaryData::nikolayevsk_namSize);
        auto model = nam::get_dsp (json);

        // assets/nam/nikolayevsk.nam e um SlimmableContainer com 2 sub-modelos (leve/full
        // quality) -- por padrao a lib ativa o mais leve (ContainerModel::_active_index{0});
        // forcamos sempre o de melhor qualidade.
        if (auto* slimmable = dynamic_cast<nam::SlimmableModel*> (model.get()))
            slimmable->SetSlimmableSize (1.0);

        return model;
    }
    catch (const std::exception& e)
    {
        logDiagnosticEvent ("NAM", juce::String ("createModel falhou -- ") + e.what());
        return nullptr;
    }
}
}

NamAmpStage::NamAmpStage() = default;
NamAmpStage::~NamAmpStage() = default;

void NamAmpStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    maxBlockSize = static_cast<int> (spec.maximumBlockSize);

    models.clear();
    scratchBuffers.clear();
    outputGainLinear = 1.0f;

    bool outputGainComputed = false;
    int loadedCount = 0;

    for (juce::uint32 ch = 0; ch < spec.numChannels; ++ch)
    {
        auto model = createModel();
        if (model != nullptr)
        {
            ++loadedCount;
            model->Reset (sampleRate, maxBlockSize);

            // So precisa calcular uma vez -- todos os canais carregam o mesmo asset, logo a
            // mesma loudness de treino.
            if (! outputGainComputed && model->HasLoudness())
            {
                constexpr double targetLoudnessDb = -18.0;
                const auto gainDb = static_cast<float> (targetLoudnessDb - model->GetLoudness());
                outputGainLinear = juce::Decibels::decibelsToGain (gainDb);
                outputGainComputed = true;
            }
        }

        models.push_back (std::move (model));
        scratchBuffers.emplace_back (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    // Resumo de uma linha so, cobrindo os dois jeitos de "deu errado": nenhum canal carregou
    // (loadedCount == 0, ve o log de createModel acima pro motivo), ou carregou mas sem
    // metadata de loudness (outputGainComputed == false -> compensacao fica em 1.0x/0dB, som
    // sai baixo igual ao bug original).
    logDiagnosticEvent ("NAM", "prepare -- canais=" + juce::String (spec.numChannels)
                                   + " carregados=" + juce::String (loadedCount)
                                   + " sampleRate=" + juce::String (sampleRate)
                                   + " loudnessCompensada=" + (outputGainComputed ? "sim" : "nao")
                                   + " outputGainDb=" + juce::String (juce::Decibels::gainToDecibels (outputGainLinear), 2));
}

void NamAmpStage::reset()
{
    for (auto& model : models)
        if (model != nullptr)
            model->Reset (sampleRate, maxBlockSize);
}

void NamAmpStage::process (juce::dsp::AudioBlock<float>& block)
{
    const auto numSamples = block.getNumSamples();
    const auto numChannels = juce::jmin (block.getNumChannels(), models.size());

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto& model = models[ch];
        if (model == nullptr)
            continue;

        auto* data = block.getChannelPointer (ch);
        auto& scratch = scratchBuffers[ch];

        std::copy (data, data + numSamples, scratch.data());

        float* inputPtr = scratch.data();
        float* outputPtr = data;
        model->process (&inputPtr, &outputPtr, static_cast<int> (numSamples));

        for (int i = 0; i < static_cast<int> (numSamples); ++i)
            data[i] *= outputGainLinear;
    }
}
