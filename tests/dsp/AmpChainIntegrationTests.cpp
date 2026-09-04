#include <catch2/catch_test_macros.hpp>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include "AmpChain.h"
#include "AmbienceStage.h"

// Regressao direta pro relato do usuario: "nem esta com reverb/delay o modo lead e clean" --
// mede energia residual (cauda de delay/reverb) alguns blocos depois do sinal de entrada ir
// para silencio total.

namespace
{
double processBurstThenSilenceAndMeasureTail (AmpChain& chain, int burstBlocks, int silenceBlocksBeforeMeasure,
                                               int blockSize, double sampleRate)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 1 };
    chain.prepare (spec);

    std::vector<float> data (static_cast<size_t> (blockSize));

    for (int b = 0; b < burstBlocks; ++b)
    {
        for (size_t i = 0; i < data.size(); ++i)
            data[i] = 0.6f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f
                                        * static_cast<float> (b * blockSize + static_cast<int> (i)) / static_cast<float> (sampleRate));

        float* channels[] = { data.data() };
        juce::dsp::AudioBlock<float> block (channels, 1, data.size());
        chain.process (block);
    }

    double tailEnergy = 0.0;
    for (int b = 0; b < silenceBlocksBeforeMeasure; ++b)
    {
        std::fill (data.begin(), data.end(), 0.0f);
        float* channels[] = { data.data() };
        juce::dsp::AudioBlock<float> block (channels, 1, data.size());
        chain.process (block);

        const auto isLastBlock = (b == silenceBlocksBeforeMeasure - 1);
        if (isLastBlock)
            for (auto sample : data)
                tailEnergy += static_cast<double> (sample) * static_cast<double> (sample);
    }

    return std::sqrt (tailEnergy / static_cast<double> (blockSize));
}
}

TEST_CASE ("AmpChain Lead mantem cauda de delay/reverb apos o sinal parar", "[ampchain][ambience]")
{
    AmpChain chain;
    chain.setMode (AmpMode::Lead);
    chain.setAmpGain (0.4f);
    chain.setBass (0.5f);
    chain.setMid (0.5f);
    chain.setTreble (0.5f);
    chain.setPresence (0.5f);
    chain.setMaster (1.0f);
    chain.setTsBypass (true);

    // A ~110ms de slapback (ver AmbienceStage) cabe dentro de 5 blocos de 512 @48k (~53ms) +
    // reverb, entao mede a cauda logo depois do silencio comecar, nao muitos blocos depois
    // (senao o decay natural do delay/reverb ja teria sumido).
    const auto tailRms = processBurstThenSilenceAndMeasureTail (chain, 20, 2, 512, 48000.0);

    INFO ("tailRms (Lead) = " << tailRms);
    REQUIRE (tailRms > 1.0e-4);
}

// NOTA: nao da pra comparar a cauda de Rhythm vs Lead no nivel do AmpChain inteiro pra provar
// que a ambiencia foi somada -- o CabinetLoader usa um IR de 0.5s (assets/ir/nikolayevsk.wav),
// entao a cauda da PROPRIA convolucao do cabinet domina totalmente qualquer medicao feita
// poucas dezenas de ms depois do sinal parar (tentativa inicial deu Rhythm=0.072 vs
// Lead=0.115 de RMS -- diferenca real, mas pequena demais pra ser um teste robusto, porque os
// dois estao dominados pelo mesmo cab). Por isso o teste abaixo isola o AmbienceStage sozinho,
// sem cabinet no meio.
TEST_CASE ("AmbienceStage com chorus desligado (config do Lead) ainda produz cauda de delay/reverb", "[ambience]")
{
    AmbienceStage ambience;
    juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
    ambience.prepare (spec);
    ambience.setChorusEnabled (false); // mesma config usada pelo AmpChain no modo Lead

    std::vector<float> left (512);
    std::vector<float> right (512);

    for (int b = 0; b < 20; ++b)
    {
        for (size_t i = 0; i < left.size(); ++i)
        {
            const auto sample = 0.6f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f
                                                  * static_cast<float> (b * 512 + static_cast<int> (i)) / 48000.0f);
            left[i] = sample;
            right[i] = sample;
        }

        float* channels[] = { left.data(), right.data() };
        juce::dsp::AudioBlock<float> block (channels, 2, left.size());
        ambience.process (block);
    }

    double tailEnergy = 0.0;
    for (int b = 0; b < 2; ++b)
    {
        std::fill (left.begin(), left.end(), 0.0f);
        std::fill (right.begin(), right.end(), 0.0f);
        float* channels[] = { left.data(), right.data() };
        juce::dsp::AudioBlock<float> block (channels, 2, left.size());
        ambience.process (block);

        if (b == 1)
            for (size_t i = 0; i < left.size(); ++i)
                tailEnergy += static_cast<double> (left[i]) * static_cast<double> (left[i]);
    }

    const auto tailRms = std::sqrt (tailEnergy / static_cast<double> (left.size()));
    INFO ("tailRms (AmbienceStage, chorus off) = " << tailRms);
    REQUIRE (tailRms > 1.0e-4);
}

TEST_CASE ("AmpChain Clean mantem cauda de ambiencia (chorus+delay+reverb) apos o sinal parar", "[ampchain][ambience]")
{
    AmpChain chain;
    chain.setMode (AmpMode::Clean);
    chain.setAmpGain (0.4f);
    chain.setBass (0.5f);
    chain.setMid (0.5f);
    chain.setTreble (0.5f);
    chain.setPresence (0.5f);
    chain.setMaster (1.0f);
    chain.setTsBypass (true);

    const auto tailRms = processBurstThenSilenceAndMeasureTail (chain, 20, 2, 512, 48000.0);

    INFO ("tailRms (Clean) = " << tailRms);
    REQUIRE (tailRms > 1.0e-4);
}
