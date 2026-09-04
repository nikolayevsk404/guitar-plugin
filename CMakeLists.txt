cmake_minimum_required(VERSION 3.22)

project(NikolayevskDarkDsp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# NOTA: fixe esta tag na versao estavel mais recente do JUCE 8.x disponivel no momento do
# build (T001) -- este valor e um ponto de partida, nao uma garantia de compatibilidade com
# o restante do CMakeLists (em especial a API de WebBrowserComponent usada em
# src/plugin/PluginEditor.cpp, que evoluiu entre releases pontuais do JUCE 8. Ver
# specs/001-nikolayevsk-dark-dsp/research.md secao 4).
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Motor de inferencia Neural Amp Modeler (assets/nam/nikolayevsk.nam, arquitetura
# SlimmableContainer/"A2" -- ver src/dsp/NamAmpStage.h). O CMakeLists.txt da propria lib nao
# expoe um target consumivel (so faz add_subdirectory(tools), que puxaria dependencias extras
# que nao usamos, como o submodule AudioDSPTools) -- por isso so populamos o source dir aqui e
# compilamos os .cpp relevantes num alvo proprio (NamCore) logo abaixo, em vez de dar
# FetchContent_MakeAvailable/add_subdirectory na lib inteira.
FetchContent_Declare(
    nam_core
    GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
    GIT_TAG v0.5.4
    GIT_SUBMODULES "Dependencies/eigen"
    GIT_SHALLOW TRUE
)
FetchContent_Populate(nam_core)

file(GLOB NAM_CORE_SOURCES
    ${nam_core_SOURCE_DIR}/NAM/*.cpp
    ${nam_core_SOURCE_DIR}/NAM/wavenet/*.cpp
)

# OBJECT (nao STATIC): varias arquiteturas da lib (ex. SlimmableContainer em NAM/container.cpp)
# se auto-registram via um objeto estatico global (ver "Auto-register" no fim de container.cpp)
# cujo construtor so roda se o .o correspondente entrar de fato no link. Uma lib STATIC (.a) so
# puxa um .o do arquivo se algum simbolo dele for referenciado -- como nada no nosso codigo
# referencia container.cpp diretamente (so chamamos nam::get_dsp() generico), o linker descarta
# esse .o e o registro nunca roda, quebrando get_dsp() pra "SlimmableContainer" com o erro "No
# config parser registered for architecture: SlimmableContainer". OBJECT forca todos os .o a
# entrarem no link, sem essa poda por simbolo.
add_library(NamCore OBJECT ${NAM_CORE_SOURCES})

target_include_directories(NamCore
    PUBLIC
        ${nam_core_SOURCE_DIR}/NAM
        ${nam_core_SOURCE_DIR}/Dependencies/eigen
        ${nam_core_SOURCE_DIR}/Dependencies/nlohmann
)

target_compile_features(NamCore PUBLIC cxx_std_20)

target_compile_definitions(NamCore
    PUBLIC
        # Usa float (igual ao juce::dsp::AudioBlock<float> do resto da cadeia) em vez do double
        # default da lib -- evita conversao por sample na fronteira com o NamAmpStage.
        NAM_SAMPLE_FLOAT
        # Path otimizado especifico pra arquitetura A2/SlimmableContainer -- exatamente o que
        # assets/nam/nikolayevsk.nam usa (ver research feito na Fase de plano).
        NAM_ENABLE_A2_FAST
        # Evita conflito com macros min/max do windows.h ao compilar no Windows; inofensivo no
        # macOS/Linux.
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

# ui/dist/index.html (React/TS via vite-plugin-singlefile, um unico HTML com JS/CSS inline)
# precisa ser buildado ANTES de configurar este CMakeLists -- roda "npm install && npm run
# build" dentro de ui/. Isso e feito como passo explicito (README.md / .github/workflows/
# ci.yml), nao como custom command do CMake: um add_custom_command chamando npm dentro do
# build MSVC multi-config se mostrou nao-confiavel no CI do Windows (o rule reportava sucesso
# sem o arquivo de saida realmente existir, quebrando a geracao do BinaryData). Falhar aqui
# com uma mensagem clara e mais simples e mais confiavel do que depender dessa integracao.
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/ui/dist/index.html)
    message(FATAL_ERROR
        "ui/dist/index.html nao encontrado. Rode antes de configurar:\n"
        "  cd ui && npm install && npm run build")
endif()

# Embutido aqui como BinaryData::index_html -- o plugin final e autocontido, sem depender de
# arquivos soltos ao lado do binario (ver PluginEditor.cpp).
juce_add_binary_data(DarkDspBinaryData
    SOURCES
        ${CMAKE_SOURCE_DIR}/assets/ir/nikolayevsk.wav
        ${CMAKE_SOURCE_DIR}/assets/nam/nikolayevsk.nam
        ${CMAKE_SOURCE_DIR}/ui/dist/index.html
)

juce_add_plugin(NikolayevskDarkDsp
    COMPANY_NAME "Nikolayevsk"
    PRODUCT_NAME "Nikolayevsk DARK DSP"
    PLUGIN_MANUFACTURER_CODE Nklv
    PLUGIN_CODE Ndd1
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx Distortion
    # So tem efeito no Windows (JUCEUtils.cmake so le isso dentro de um bloco elseif(WIN32)) --
    # sem isso juce_gui_extra.cpp nao acha WebView2.h ao compilar com JUCE_USE_WIN_WEBVIEW2=1.
    # Precisa do pacote NuGet Microsoft.Web.WebView2 disponivel via JUCE_WEBVIEW2_PACKAGE_LOCATION
    # (ver .github/workflows/ci.yml e README.md para o passo manual local no Windows).
    NEEDS_WEBVIEW2 TRUE
)

target_sources(NikolayevskDarkDsp
    PRIVATE
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginEditor.cpp
        src/dsp/AmpChain.cpp
        src/dsp/TubeScreamerStage.cpp
        src/dsp/PreampStage.cpp
        src/dsp/ToneStack.cpp
        src/dsp/PowerAmpSaturation.cpp
        src/dsp/CabinetLoader.cpp
        src/dsp/AmbienceStage.cpp
        src/dsp/NamAmpStage.cpp
)

target_compile_definitions(NikolayevskDarkDsp
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        # No Windows, o backend default do WebBrowserComponent e o Internet Explorer, que nao
        # suporta withResourceProvider (so compila -- e so existe -- com o backend WebView2
        # habilitado). Sem isso o build do Windows falha com "'withResourceProvider': is not a
        # member of 'juce::WebBrowserComponent::Options'".
        JUCE_USE_WIN_WEBVIEW2=1
        # CONFIRMADO EM MAQUINA REAL (log de PluginEditor.cpp): com carregamento dinamico (sem
        # esta flag), o runtime faz LoadLibraryA("WebView2Loader.dll") e nao acha o arquivo (a
        # gente nunca empacotou essa DLL do lado do binario) -- a criacao do WebView2 falha
        # silenciosamente e o JUCE cai de volta pro Internet Explorer (log mostrou literalmente
        # "res://ieframe.dll/navcancl.htm"). Linkagem estatica usa direto o
        # WebView2LoaderStatic.lib que o NEEDS_WEBVIEW2 abaixo ja linka -- sem precisar
        # distribuir DLL nenhuma junto do plugin.
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

target_link_libraries(NikolayevskDarkDsp
    PRIVATE
        DarkDspBinaryData
        NamCore
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()
add_subdirectory(tests)
