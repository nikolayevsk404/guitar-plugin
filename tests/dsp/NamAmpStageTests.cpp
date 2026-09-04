#include <catch2/catch_test_macros.hpp>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include "NamAmpStage.h"

// Cobre o motor NAM (assets/nam/nikolayevsk.nam) usado em Rhythm/Lead: robustez a troca de
// sample rate/buffer size (paridade com NFR-003, ja cobrado pro CabinetLoader), ausencia de
// NaN/Inf e independencia de estado entre canais.

TEST_CASE ("NamAmpStage silencio na entrada nao produz NaN/Inf", "[namamp]")
{
    NamAmpStage stage;
    juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
    stage.prepare (spec);

    std::vector<float> left (512, 0.0f);
    std::vector<float> right (512, 0.0f);
    float* channels[] = { left.data(), right.data() };
    juce::dsp::AudioBlock<float> block (channels, 2, 512);

    stage.process (block);

    for (auto sample : left)
        REQUIRE (std::isfinite (sample));
    for (auto sample : right)
        REQUIRE (std::isfinite (sample));
}

TEST_CASE ("NamAmpStage processa seno sem lancar excecao e sem amplitude absurda", "[namamp]")
{
    NamAmpStage stage;
    juce::dsp::ProcessSpec spec { 48000.0, 256, 1 };
    stage.prepare (spec);

    std::vector<float> data (256);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 110.0f * static_cast<float> (i) / 48000.0f);

    float* channels[] = { data.data() };
    juce::dsp::AudioBlock<float> block (channels, 1, data.size());

    REQUIRE_NOTHROW (stage.process (block));

    for (auto sample : data)
    {
        REQUIRE (std::isfinite (sample));
        REQUIRE (std::abs (sample) < 100.0f);
    }
}

TEST_CASE ("NamAmpStage sobrevive a troca de sample rate e buffer size", "[namamp]")
{
    NamAmpStage stage;

    for (auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        for (auto blockSize : { 64, 256, 1024 })
        {
            juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };
            stage.prepare (spec);
            stage.reset();

            std::vector<float> left (static_cast<size_t> (blockSize), 0.1f);
            std::vector<float> right (static_cast<size_t> (blockSize), -0.1f);
            float* channels[] = { left.data(), right.data() };
            juce::dsp::AudioBlock<float> block (channels, 2, static_cast<size_t> (blockSize));

            REQUIRE_NOTHROW (stage.process (block));
        }
    }
}

TEST_CASE ("NamAmpStage altera um seno de forma substancial (nao e passthrough)", "[namamp]")
{
    // Regressao direta pro relato "e como se nao tivessemos processado nada" -- assets/nam/
    // nikolayevsk.nam e uma captura de cabecote de alto gain (referencia do usuario: "5150"),
    // entao um seno de entrada deve sair bem distorcido/comprimido, nao uma copia quase igual.
    NamAmpStage stage;
    juce::dsp::ProcessSpec spec { 48000.0, 512, 1 };
    stage.prepare (spec);

    std::vector<float> input (512);
    for (size_t i = 0; i < input.size(); ++i)
        input[i] = 0.6f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f * static_cast<float> (i) / 48000.0f);

    // Processa alguns blocos iguais pra passar do transiente inicial (prewarm/receptive field
    // da WaveNet) antes de medir -- so o ultimo bloco processado e comparado com a entrada.
    std::vector<float> data;
    for (int blockIdx = 0; blockIdx < 4; ++blockIdx)
    {
        data = input;
        float* channels[] = { data.data() };
        juce::dsp::AudioBlock<float> block (channels, 1, data.size());
        stage.process (block);
    }

    double sumAbsDiff = 0.0;
    for (size_t i = 0; i < input.size(); ++i)
        sumAbsDiff += std::abs (static_cast<double> (data[i]) - static_cast<double> (input[i]));
    const auto meanAbsDiff = sumAbsDiff / static_cast<double> (input.size());

    INFO ("meanAbsDiff = " << meanAbsDiff);
    REQUIRE (meanAbsDiff > 0.02);
}

TEST_CASE ("NamAmpStage nao mistura estado entre canais", "[namamp]")
{
    NamAmpStage stage;
    juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
    stage.prepare (spec);

    std::vector<float> left (512, 0.3f);
    std::vector<float> right (512, 0.0f);
    float* channels[] = { left.data(), right.data() };
    juce::dsp::AudioBlock<float> block (channels, 2, 512);

    stage.process (block);

    // Canal direito entrou em silencio absoluto -- se o motor nao misturar estado entre
    // canais, a saida dele deve continuar em (perto de) silencio mesmo com o esquerdo ativo.
    for (auto sample : right)
        REQUIRE (std::abs (sample) < 0.05f);
}

TEST_CASE ("NamAmpStage aplica compensacao de loudness (nao fica baixo demais)", "[namamp]")
{
    // Regressao pro relato "som capenga/fraco em Rhythm/Lead": sem compensar a loudness de
    // treino do .nam (metadata "loudness", ~-24dB no nikolayevsk.nam), o motor processa
    // corretamente mas sai bem mais baixo que a entrada. Mesma formula do modo "Normalized"
    // do plugin oficial NeuralAmpModelerPlugin (targetLoudness = -18dB).
    NamAmpStage stage;
    juce::dsp::ProcessSpec spec { 48000.0, 512, 1 };
    stage.prepare (spec);

    std::vector<float> input (512);
    for (size_t i = 0; i < input.size(); ++i)
        input[i] = 0.6f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f * static_cast<float> (i) / 48000.0f);

    std::vector<float> data;
    for (int blockIdx = 0; blockIdx < 20; ++blockIdx)
    {
        data = input;
        float* channels[] = { data.data() };
        juce::dsp::AudioBlock<float> block (channels, 1, data.size());
        stage.process (block);
    }

    double sumSqOut = 0.0, sumSqIn = 0.0;
    for (size_t i = 0; i < data.size(); ++i)
    {
        sumSqOut += static_cast<double> (data[i]) * data[i];
        sumSqIn += static_cast<double> (input[i]) * input[i];
    }
    const auto outRms = std::sqrt (sumSqOut / data.size());
    const auto inRms = std::sqrt (sumSqIn / data.size());
    const auto ratioDb = 20.0 * std::log10 (outRms / inRms);

    INFO ("ratioDb = " << ratioDb);
    REQUIRE (ratioDb > -10.0);
}
