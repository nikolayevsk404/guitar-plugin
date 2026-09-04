#include "PluginEditor.h"
#include "BinaryData.h"
#include "../util/DiagnosticLog.h"

namespace
{
    // Diagnostico temporario (nao e telemetria -- so escreve local, nada e enviado a lugar
    // nenhum): o WebView2 no Windows mostrou a pagina de erro nativa do Edge em vez de
    // servir o resourceProvider, mesmo com getResourceProviderRoot() batendo exatamente com
    // o que juce_WebBrowserComponent_windows.cpp espera. Sem uma maquina Windows pra debugar
    // ao vivo, este log e o jeito de descobrir em qual ponto exato a cadeia falha (provider
    // nunca chamado? chamado mas com erro? erro de rede real do WebView2?).
    void logWebViewEvent (const juce::String& message)
    {
        logDiagnosticEvent ("WebView", message);
    }

    class DiagnosticWebBrowser : public juce::WebBrowserComponent
    {
    public:
        using juce::WebBrowserComponent::WebBrowserComponent;

        bool pageAboutToLoad (const juce::String& newURL) override
        {
            logWebViewEvent ("pageAboutToLoad: " + newURL);
            return juce::WebBrowserComponent::pageAboutToLoad (newURL);
        }

        void pageFinishedLoading (const juce::String& url) override
        {
            logWebViewEvent ("pageFinishedLoading: " + url);
            juce::WebBrowserComponent::pageFinishedLoading (url);
        }

        bool pageLoadHadNetworkError (const juce::String& errorInfo) override
        {
            logWebViewEvent ("pageLoadHadNetworkError: " + errorInfo);
            return juce::WebBrowserComponent::pageLoadHadNetworkError (errorInfo);
        }

        void newWindowAttemptingToLoad (const juce::String& newURL) override
        {
            logWebViewEvent ("newWindowAttemptingToLoad: " + newURL);
            juce::WebBrowserComponent::newWindowAttemptingToLoad (newURL);
        }
    };
}

PluginEditor::PluginEditor (PluginProcessor& processorRef)
    : juce::AudioProcessorEditor (processorRef), pluginProcessor (processorRef)
{
    logWebViewEvent ("PluginEditor construído -- BinaryData index_html size = "
                      + juce::String (BinaryData::index_htmlSize));

    // CONFIRMADO EM MAQUINA REAL: mesmo com linkagem estatica, o WebView2 ainda caia pro IE.
    // A doc do proprio JUCE avisa exatamente sobre isso -- em plugins, o WebView2 tenta criar
    // sua pasta de user-data do lado do executavel do HOST (o Reaper, normalmente dentro de
    // Program Files, sem permissao de escrita), a criacao do ambiente falha silenciosamente e
    // cai pro IE. Apontando pra uma pasta com permissao de escrita garantida.
    const auto webView2UserDataFolder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                             .getChildFile ("NikolayevskDarkDSP")
                                             .getChildFile ("WebView2UserData");
    webView2UserDataFolder.createDirectory();
    logWebViewEvent ("webView2UserDataFolder = " + webView2UserDataFolder.getFullPathName()
                      + " (exists=" + (webView2UserDataFolder.isDirectory() ? "sim" : "nao") + ")");

    webView = std::make_unique<DiagnosticWebBrowser> (
        juce::WebBrowserComponent::Options {}
            // No Windows, precisa ser explicito -- o default e Internet Explorer. Em
            // plataformas onde webview2 nao existe (Mac/Linux), o JUCE cai de volta pro
            // backend default silenciosamente (WebBrowserComponent::areOptionsSupported).
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2 {}
                    .withUserDataFolder (webView2UserDataFolder))
            .withNativeIntegrationEnabled()
            .withResourceProvider ([this] (const juce::String& url) { return provideResource (url); })
            .withNativeFunction ("setParam",
                [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    completion (handleSetParam (args));
                })
            .withNativeFunction ("getState",
                [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    completion (buildStateVar());
                })
            .withNativeFunction ("loadIR",
                [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    completion (handleLoadIr (args));
                })
            .withNativeFunction ("browseForIR",
                [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    fileChooser = std::make_unique<juce::FileChooser> (
                        "Selecione um Impulse Response (.wav)", juce::File {}, "*.wav");

                    fileChooser->launchAsync (
                        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, completion] (const juce::FileChooser& fc)
                        {
                            const auto file = fc.getResult();
                            if (file != juce::File {})
                            {
                                const auto success = pluginProcessor.loadIrFromFile (file); // false = mantem IR anterior (edge case da spec)
                                logDiagnosticEvent ("IR", "browseForIR -- arquivo=" + file.getFullPathName()
                                                              + " sucesso=" + (success ? "sim" : "nao"));
                            }
                            else
                            {
                                logDiagnosticEvent ("IR", "browseForIR -- usuario cancelou o dialogo");
                            }

                            completion (buildIrStateVar());
                        });
                }));

    addAndMakeVisible (*webView);
    // getResourceProviderRoot() e o unico jeito correto de apontar o WebBrowserComponent para
    // o resourceProvider registrado acima -- uma URL arbitraria (mesmo com dominio .invalid)
    // tenta resolver DNS de verdade e falha ("A server with the specified hostname could not
    // be found"). Confirmado lendo o header real: modules/juce_gui_extra/misc/
    // juce_WebBrowserComponent.h em build/_deps/juce-src apos o primeiro build.
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    setSize (840, 560);
}

PluginEditor::~PluginEditor() = default;

void PluginEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> PluginEditor::provideResource (const juce::String& url)
{
    // App React e uma SPA de arquivo unico (ui/vite.config.ts usa vite-plugin-singlefile) --
    // qualquer URL pedida recebe o mesmo index.html embutido como BinaryData.
    logWebViewEvent ("provideResource chamado com url = " + url);
    return juce::WebBrowserComponent::Resource {
        std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (BinaryData::index_html),
            reinterpret_cast<const std::byte*> (BinaryData::index_html) + BinaryData::index_htmlSize),
        "text/html"
    };
}

juce::var PluginEditor::handleSetParam (const juce::Array<juce::var>& args)
{
    // Diagnostico temporario (mesmo esquema do log do WebView acima) -- pra confirmar se a UI
    // esta mesmo chamando setParam("ampMode", ...) quando o usuario clica Rhythm/Lead, ja que
    // o relato foi "nada muda de som ao trocar de modo".
    logWebViewEvent ("handleSetParam chamado -- args.size()=" + juce::String (args.size())
                      + (args.size() > 0 ? (" id=" + args[0].toString()) : juce::String())
                      + (args.size() > 1 ? (" value=" + args[1].toString()) : juce::String()));

    if (args.size() < 2)
        return false;

    const auto id = args[0].toString();

    if (id == ParamId::ampMode)
    {
        const auto modeStr = args[1].toString();
        const int index = modeStr == "rhythm" ? 1 : (modeStr == "lead" ? 2 : 0);
        if (auto* param = pluginProcessor.apvts.getParameter (ParamId::ampMode))
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (index)));
        return true;
    }

    if (auto* param = pluginProcessor.apvts.getParameter (id))
    {
        param->setValueNotifyingHost (static_cast<float> (static_cast<double> (args[1])));
        return true;
    }

    return false;
}

juce::var PluginEditor::handleLoadIr (const juce::Array<juce::var>& args)
{
    if (args.isEmpty())
        return buildIrStateVar();

    const juce::File file (args[0].toString());
    const auto success = pluginProcessor.loadIrFromFile (file); // false = mantem IR anterior (edge case da spec)
    logDiagnosticEvent ("IR", "handleLoadIr -- arquivo=" + file.getFullPathName()
                                  + " sucesso=" + (success ? "sim" : "nao"));
    return buildIrStateVar();
}

juce::var PluginEditor::ampModeToVar (AmpMode mode)
{
    switch (mode)
    {
        case AmpMode::Clean:  return "clean";
        case AmpMode::Rhythm: return "rhythm";
        case AmpMode::Lead:   return "lead";
    }
    return "clean";
}

juce::var PluginEditor::buildStateVar() const
{
    auto* obj = new juce::DynamicObject();

    const auto modeIndex = static_cast<int> (pluginProcessor.apvts.getRawParameterValue (ParamId::ampMode)->load());
    obj->setProperty ("ampMode", ampModeToVar (static_cast<AmpMode> (modeIndex)));
    obj->setProperty ("gain", pluginProcessor.apvts.getRawParameterValue (ParamId::gain)->load());
    obj->setProperty ("bass", pluginProcessor.apvts.getRawParameterValue (ParamId::bass)->load());
    obj->setProperty ("mid", pluginProcessor.apvts.getRawParameterValue (ParamId::mid)->load());
    obj->setProperty ("treble", pluginProcessor.apvts.getRawParameterValue (ParamId::treble)->load());
    obj->setProperty ("presence", pluginProcessor.apvts.getRawParameterValue (ParamId::presence)->load());
    obj->setProperty ("master", pluginProcessor.apvts.getRawParameterValue (ParamId::master)->load());
    obj->setProperty ("tsDrive", pluginProcessor.apvts.getRawParameterValue (ParamId::tsDrive)->load());
    obj->setProperty ("tsTone", pluginProcessor.apvts.getRawParameterValue (ParamId::tsTone)->load());
    obj->setProperty ("tsLevel", pluginProcessor.apvts.getRawParameterValue (ParamId::tsLevel)->load());
    obj->setProperty ("tsBypass", pluginProcessor.apvts.getRawParameterValue (ParamId::tsBypass)->load() > 0.5f);

    return juce::var (obj);
}

juce::var PluginEditor::buildIrStateVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("loaded", true);
    obj->setProperty ("name", pluginProcessor.getCurrentIrName());
    obj->setProperty ("isDefault", pluginProcessor.isUsingDefaultIr());
    return juce::var (obj);
}
