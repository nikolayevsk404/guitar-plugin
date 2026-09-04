#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../util/DiagnosticLog.h"

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    ampChain.prepare (spec);
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == mono || mainOut == stereo;
}

void PluginProcessor::pullParametersIntoAmpChain()
{
    const auto modeIndex = static_cast<int> (apvts.getRawParameterValue (ParamId::ampMode)->load());
    ampChain.setMode (static_cast<AmpMode> (modeIndex));

    ampChain.setAmpGain (apvts.getRawParameterValue (ParamId::gain)->load());
    ampChain.setBass (apvts.getRawParameterValue (ParamId::bass)->load());
    ampChain.setMid (apvts.getRawParameterValue (ParamId::mid)->load());
    ampChain.setTreble (apvts.getRawParameterValue (ParamId::treble)->load());
    ampChain.setPresence (apvts.getRawParameterValue (ParamId::presence)->load());
    ampChain.setMaster (apvts.getRawParameterValue (ParamId::master)->load());

    ampChain.setTsDrive (apvts.getRawParameterValue (ParamId::tsDrive)->load());
    ampChain.setTsTone (apvts.getRawParameterValue (ParamId::tsTone)->load());
    ampChain.setTsLevel (apvts.getRawParameterValue (ParamId::tsLevel)->load());
    ampChain.setTsBypass (apvts.getRawParameterValue (ParamId::tsBypass)->load() > 0.5f);
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    pullParametersIntoAmpChain();

    juce::dsp::AudioBlock<float> block (buffer);
    ampChain.process (block);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

bool PluginProcessor::loadIrFromFile (const juce::File& file)
{
    const auto success = ampChain.loadIrFromFile (file);
    if (success)
        loadedIrPath = file.getFullPathName();
    return success;
}

void PluginProcessor::loadDefaultIr()
{
    ampChain.loadDefaultIr();
    loadedIrPath.clear();
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    xml->setAttribute ("loadedIrPath", loadedIrPath);
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr)
    {
        logDiagnosticEvent ("State", "setStateInformation -- xml nulo (sizeInBytes=" + juce::String (sizeInBytes) + "), ignorando");
        return;
    }

    apvts.replaceState (juce::ValueTree::fromXml (*xml));

    const auto irPath = xml->getStringAttribute ("loadedIrPath");
    logDiagnosticEvent ("State", "setStateInformation -- loadedIrPath salvo='" + irPath + "'");
    if (irPath.isNotEmpty())
    {
        // Fallback do edge case da spec: se o IR salvo no preset nao existir mais nesta
        // maquina, cai para o IR default embutido em vez de ficar "mudo".
        if (! loadIrFromFile (juce::File (irPath)))
            loadDefaultIr();
    }
    else
    {
        loadDefaultIr();
    }
}

// Ponto de entrada exigido pelo JUCE para instanciar o AudioProcessor.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
