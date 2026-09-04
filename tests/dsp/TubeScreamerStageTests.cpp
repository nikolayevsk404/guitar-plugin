#include <catch2/catch_test_macros.hpp>
#include <juce_dsp/juce_dsp.h>
#include "TubeScreamerStage.h"

// Cobre os criterios de aceite de T010 (tasks.md): bypass e bit-perfeito; drive alto produz
// clipping visivel sem sair do range esperado de um waveshaper tanh.

TEST_CASE ("TubeScreamerStage bypass e bit-perfeito", "[tubescreamer]")
{
    TubeScreamerStage stage;
    juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
    stage.prepare (spec);
    stage.setBypass (true);
    stage.setDrive (1.0f);

    std::vector<float> input { 0.1f, -0.2f, 0.3f, -0.4f, 0.5f };
    std::vector<float> data = input;
    float* channels[] = { data.data() };
    juce::dsp::AudioBlock<float> block (channels, 1, data.size());

    stage.process (block);

    for (size_t i = 0; i < input.size(); ++i)
        REQUIRE (data[i] == input[i]);
}

TEST_CASE ("TubeScreamerStage com drive alto produz clipping limitado", "[tubescreamer]")
{
    TubeScreamerStage stage;
    juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
    stage.prepare (spec);
    stage.reset();
    stage.setBypass (false);
    stage.setDrive (1.0f);
    stage.setTone (0.5f);
    stage.setLevel (1.0f);

    std::vector<float> data (256, 0.9f);
    float* channels[] = { data.data() };
    juce::dsp::AudioBlock<float> block (channels, 1, data.size());

    stage.process (block);

    for (auto sample : data)
        REQUIRE (std::abs (sample) <= 1.0f);
}
