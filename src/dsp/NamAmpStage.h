#pragma once

#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace nam
{
class DSP;
}

// Motor de cabecote via captura Neural Amp Modeler (assets/nam/nikolayevsk.nam), usado em
// Rhythm/Lead no lugar de PreampStage+PowerAmpSaturation (AmpChain.cpp decide o roteamento;
// Clean continua na cadeia analogica). Cada canal tem sua propria instancia de nam::DSP -- a
// lib guarda estado de convolucao interno por instancia, entao compartilhar uma so entre
// canais misturaria o estado de L/R (mesmo motivo de PreampStage usar ProcessorDuplicator por
// canal em vez de um filtro so).
class NamAmpStage
{
public:
    // Declarado fora de linha (definido em NamAmpStage.cpp, onde nam::DSP e um tipo completo)
    // -- necessario porque std::unique_ptr<nam::DSP> precisa do tipo completo no ponto onde o
    // destrutor e instanciado, e aqui so temos a forward declaration.
    NamAmpStage();
    ~NamAmpStage();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

private:
    double sampleRate = 44100.0;
    int maxBlockSize = 512;

    // Um modelo por canal; ponteiro nulo se o asset embutido falhar ao carregar (process()
    // vira passthrough nesse canal -- nunca deixa o plugin mudo/crashado por asset ruim, mesma
    // filosofia de fallback do CabinetLoader).
    std::vector<std::unique_ptr<nam::DSP>> models;
    std::vector<std::vector<float>> scratchBuffers;

    // Ganho de saida (linear) pra compensar a loudness de treino do .nam -- sem isso, o motor
    // processa/distorce o sinal corretamente mas sai bem mais baixo que a entrada (medido
    // ~-13dB com nikolayevsk.nam), o que da a sensacao de "nao esta pegando o som do cabecote".
    // Mesma formula do modo "Normalized" do plugin oficial NeuralAmpModelerPlugin
    // (NeuralAmpModeler.cpp, _SetOutputGain(), targetLoudness = -18dB): ganhoDB = -18 -
    // loudness_do_modelo.
    float outputGainLinear = 1.0f;
};
