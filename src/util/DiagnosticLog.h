#pragma once

#include <juce_core/juce_core.h>

// Diagnostico temporario e 100% local (nada e enviado a lugar nenhum, so escreve nesse
// arquivo) -- compartilhado por PluginEditor, NamAmpStage e CabinetLoader pra registrar
// eventos de ciclo de vida (carregamento de modelo/IR, chamadas da UI) num unico arquivo,
// pra dar pra cruzar tudo numa sessao so de teste. Ver relato "som capenga em Rhythm/Lead,
// como se nao tivesse pegando o cabecote/IR de verdade".
//
// NUNCA chamar isso de dentro de processBlock()/AmpChain::process() ou qualquer coisa que
// rode por bloco de audio -- I/O de arquivo na thread de audio quebra o realtime. So e seguro
// em pontos de ciclo de vida (construtor, prepareToPlay/prepare(), callbacks de UI).
inline void logDiagnosticEvent (const juce::String& component, const juce::String& message)
{
    const auto logFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("NikolayevskDarkDSP_webview.log");
    logFile.appendText (juce::Time::getCurrentTime().toString (true, true, true, true)
                         + "  [" + component + "] " + message + "\n");
}
