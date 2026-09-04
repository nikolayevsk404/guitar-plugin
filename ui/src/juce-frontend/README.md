# juce-frontend (vendorizado)

`index.js` e `check_native_interop.js` sao copias exatas, sem modificacao, do pacote oficial
`juce-framework-frontend` que vem dentro do proprio JUCE em
`modules/juce_gui_extra/native/javascript/`. Licenca original preservada no cabecalho de cada
arquivo (termos do JUCE framework, com opcao AGPLv3) -- usado aqui sob os mesmos termos de
licenca open source do JUCE que o resto deste projeto ja usa (GPLv3, ver `LICENSE` na raiz).

Vendorizamos em vez de referenciar via `file:../../build/_deps/juce-src/...` (como o exemplo
oficial `WebViewPluginDemoGUI` faz) porque esse caminho so existe depois de rodar o CMake pelo
menos uma vez -- vendorizar deixa `ui/` buildavel de forma independente.

Se a tag do JUCE em `CMakeLists.txt` (raiz do repo) for atualizada, revalidar se esses dois
arquivos mudaram na nova versao e reexportar de
`build/_deps/juce-src/modules/juce_gui_extra/native/javascript/`.

API usada por `ui/src/bridge/nativeBridge.ts`: `getNativeFunction`. As outras exports
(`getSliderState`, `getToggleState`, `getComboBoxState`, `ControlParameterIndexUpdater`) nao
sao usadas -- este projeto expoe parametros via funcoes nativas customizadas
(setParam/getState/loadIR/browseForIR), nao via os relays de parametro prontos do JUCE.
