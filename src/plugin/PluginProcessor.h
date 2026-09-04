#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterLayout.h"
#include "../dsp/AmpChain.h"

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nikolayevsk DARK DSP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool loadIrFromFile (const juce::File& file);
    void loadDefaultIr();
    juce::String getCurrentIrName() const { return ampChain.getCurrentIrName(); }
    bool isUsingDefaultIr() const { return ampChain.isUsingDefaultIr(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    void pullParametersIntoAmpChain();

    AmpChain ampChain;
    juce::String loadedIrPath; // vazio = usando o IR default embutido (FR-008/T040)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
