#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// Host do WebView (React/TS embutido) + ponte nativa. Contrato de mensagens documentado em
// ui/src/bridge/nativeBridge.ts e specs/001-nikolayevsk-dark-dsp/research.md secao 4.
//
// ATENCAO: a API de juce::WebBrowserComponent usada aqui (withNativeIntegrationEnabled,
// withResourceProvider, withNativeFunction, NativeFunctionCompletion) e a forma mais comum
// documentada nos exemplos oficiais do JUCE 8 no momento da escrita, mas MUDOU entre
// releases pontuais -- valide contra a tag do JUCE fixada em CMakeLists.txt antes de
// considerar T021/T022 (tasks.md) concluidas.
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor& processorRef);
    ~PluginEditor() override;

    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> provideResource (const juce::String& url);

    juce::var handleSetParam (const juce::Array<juce::var>& args);
    juce::var handleLoadIr (const juce::Array<juce::var>& args);
    juce::var buildStateVar() const;
    juce::var buildIrStateVar() const;
    static juce::var ampModeToVar (AmpMode mode);

    PluginProcessor& pluginProcessor;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
