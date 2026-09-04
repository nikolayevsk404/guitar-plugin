# Implementation Plan: Nikolayevsk DARK DSP

**Input**: `spec.md` (mesma pasta)
**Status**: Draft — depende de aprovação antes de gerar `tasks.md` executável

## Technical Context

| Item | Decisão | Racional |
|---|---|---|
| Linguagem DSP/host | C++20 | Requisito do JUCE 8; performance de áudio em tempo real |
| Framework de plugin | JUCE 8.x (via CMake FetchContent, não Projucer) | JUCE grátis sob GPLv3 (decisão do usuário); CMake é reprodutível em CI e não depende de IDE |
| Formato de build | VST3 apenas (decisão do usuário) | AU/AAX fora de escopo nesta fase |
| UI | React 18 + TypeScript, bundlado com Vite, servido dentro de `juce::WebBrowserComponent` | Aproveita o stack forte do usuário (React/TS) sem escrever UI em C++/JUCE Components |
| Ponte UI↔DSP | `juce::WebBrowserComponent` com funções nativas registradas (`withNativeIntegrationEnabled` + `withOptionsFrom`/native function bindings do JUCE 8) trocando JSON | Único mecanismo suportado nativamente pelo JUCE 8 para host de WebView com call nativo bidirecional, sem servidor HTTP local |
| Parâmetros | `juce::AudioProcessorValueTreeState` (APVTS) como fonte única de verdade | Garante automação de host, undo do host, thread-safety de parâmetro já resolvidos pelo framework |
| IR / Convolução | `juce::dsp::Convolution` | Já lida com fase mínima/linear, streaming, e é parte do módulo `juce_dsp` — evita dependência externa |
| Testes DSP | Catch2 (unit tests de estágios de processamento isolados) | Padrão de facto em projetos JUCE/C++ áudio |
| Validação de plugin | `pluginval` (nível estrito) em CI | Ferramenta padrão da comunidade JUCE para detectar crash/threading/state bugs |
| CI | GitHub Actions, matriz `windows-latest` + `macos-latest` | Cobre NFR-004 (portabilidade) sem exigir que o usuário tenha as duas máquinas |
| Build de dependências nativas | CMake `FetchContent` para JUCE; `npm`/Vite para o app React, cujo `dist/` é empacotado como `BinaryData` do JUCE | Um único `cmake --build` deve gerar o VST3 final, sem passo manual de copiar arquivos |

## Naming / Trademark Guardrail (deriva de spec.md)

Nenhum identificador público (nome do plugin, nomes de parâmetro, nomes de preset, strings de
UI, metadados VST3) pode conter "Marshall", "JCM800", "Mesa", "Mesa Boogie" ou variações.
Nomenclatura interna aprovada para os dois voicings:

- Voicing britânico de alto gain → identificador interno `BRIT_HI`, label de UI **"Brit Crunch"**
- Voicing americano de gain em cascata → identificador interno `CALI_HI`, label de UI **"Cali Lift"**

Isso é regra de código (constantes/enums), não só de texto solto — deve ser validada em code
review antes de qualquer release pública.

## Architecture Overview

```
┌─────────────────────────────── VST3 Plugin Process ───────────────────────────────┐
│                                                                                      │
│  ┌──────────────────────────┐        JSON messages (get/set param, load IR)        │
│  │  React + TS UI (Vite)    │ <───────────────────────────────────────────────┐    │
│  │  rodando dentro de       │                                                 │    │
│  │  juce::WebBrowserComponent│ ───────────────────────────────────────────────┼──┐ │
│  └──────────────────────────┘                                                 │  │ │
│                                                                                 ▼  │ │
│                                                              ┌──────────────────────┐│
│                                                              │  PluginEditor (JUCE) ││
│                                                              │  (host do WebView +  ││
│                                                              │   native functions)  ││
│                                                              └──────────┬───────────┘│
│                                                                          │ APVTS      │
│                                                              ┌──────────▼───────────┐│
│                                                              │   PluginProcessor    ││
│                                                              │  (AudioProcessor)    ││
│                                                              └──────────┬───────────┘│
│                                                                          │            │
│   Cadeia de processamento de áudio (processBlock), por amostra/bloco:  │            │
│                                                                          ▼            │
│   InputTrim → NoiseGate → TubeScreamerStage → [Clean|Rhythm|Lead]PreampStage         │
│   → ToneStack(Bass/Mid/Treble/Presence) → PowerAmpSaturation → CabinetConvolution    │
│   → OutputTrim                                                                       │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

Princípio-chave: **a UI nunca processa áudio e o DSP nunca depende da UI estar viva.** Se o
WebView falhar ao carregar, o `AudioProcessor` continua rodando com o último estado válido do
APVTS (endereça o edge case da spec sobre falha de WebView).

## DSP Chain — detalhamento por estágio

1. **InputTrim**: gain de entrada simples (compensar níveis de interface de áudio distintas).
2. **NoiseGate**: gate simples (threshold/release) — necessário porque Rhythm/Lead têm ganho
   alto e chiado de fundo é inaceitável num plugin "signature".
3. **TubeScreamerStage**: estágio de op-amp soft-clip modelado (waveshaper com curva assimétrica
   leve + filtro de tone pré-clipping), parâmetros Drive/Tone/Level + bypass true/false
   (bypass real, não "gain zero" — para não colorir o sinal quando desligado).
4. **PreambStage (por modo)**: 3 variantes de curva de saturação + gain staging,
   selecionadas pelo `AmpMode` (Clean/Rhythm/Lead) — implementadas como estratégia (uma classe
   por modo ou uma função parametrizada, decisão de implementação em tasks.md).
5. **ToneStack**: filtros IIR (shelving/peaking) para Bass/Mid/Treble/Presence — resposta
   inspirada na tabela "Voicing Target" da spec, não um clone literal de circuito.
6. **PowerAmpSaturation**: segundo estágio de waveshaping mais sutil, simula "power tube sag"
   em Rhythm/Lead — contribui para a sensação de resposta dinâmica que a spec pede.
7. **CabinetConvolution**: `juce::dsp::Convolution` com IR embutido default + suporte a IR
   custom do usuário (FR-005/FR-006).
8. **OutputTrim**: master volume final, com metering para a UI (VU-meter, FR-V04).

Todo estágio exposto por parâmetro DEVE usar `juce::SmoothedValue` (endereça FR-012 — sem
estalos em automação/troca de modo).

## Project Structure

```
guitar-plugin/
├── LICENSE                      # GPLv3 (já criado)
├── README.md
├── CMakeLists.txt               # root build — FetchContent do JUCE, subdiretórios
├── specs/
│   └── 001-nikolayevsk-dark-dsp/
│       ├── spec.md
│       ├── plan.md              # este arquivo
│       ├── research.md
│       └── tasks.md
├── src/
│   ├── dsp/
│   │   ├── AmpChain.h/.cpp          # orquestra os estágios
│   │   ├── TubeScreamerStage.h/.cpp
│   │   ├── PreampStage.h/.cpp       # 3 modos
│   │   ├── ToneStack.h/.cpp
│   │   ├── PowerAmpSaturation.h/.cpp
│   │   └── CabinetLoader.h/.cpp     # wrapper de juce::dsp::Convolution + gestão de IR
│   ├── plugin/
│   │   ├── PluginProcessor.h/.cpp
│   │   ├── PluginEditor.h/.cpp      # host do WebBrowserComponent + native functions
│   │   └── ParameterLayout.h/.cpp   # definição única do APVTS
│   └── BinaryData/                  # gerado: IR default embutido + dist/ do React
├── ui/                           # app React + TypeScript (Vite)
│   ├── src/
│   │   ├── components/
│   │   │   ├── AmpHead.tsx
│   │   │   ├── Cabinet.tsx
│   │   │   ├── TubeScreamerPedal.tsx
│   │   │   └── ModeSelector.tsx
│   │   ├── bridge/nativeBridge.ts   # wrapper typed sobre as funções nativas do JUCE
│   │   └── theme/                   # tokens visuais gótico/rústico (cor, textura, tipografia)
│   └── package.json
├── assets/
│   └── ir/default_cab.wav       # IR default embutido
├── tests/
│   └── dsp/                     # Catch2 — um arquivo por estágio de DSP
└── .github/workflows/ci.yml     # build matrix Windows + macOS, roda pluginval
```

## Complexity Tracking

Nenhum desvio de complexidade identificado nesta fase — arquitetura é a mínima necessária para
atender FR-001 a FR-013. Reavaliar se, na implementação, `PreampStage` precisar de estado
compartilhado entre modos que force acoplamento maior que o previsto aqui.

## Open Questions para antes de gerar tasks.md executável

1. **IR default embutido**: precisamos de um arquivo .wav de IR de cabinet real (capturado ou
   de terceiros com licença compatível) para o preset plug-and-play (FR-009). Isso não pode ser
   gerado por código — precisa ser fornecido ou capturado por você, ou usar um IR livre de
   licença permissiva. Fica pendente para antes da Fase de packaging.
2. **Assets de arte final** (logo, textura, ícones da UI) dependem da Haruna Lamberti (FR-V03)
   — o scaffold vai usar placeholders até os assets reais chegarem.

## Review & Acceptance Checklist

- [x] Toda decisão técnica tem racional ligado a um requisito de `spec.md`
- [x] Guardrail de trademark tratado como regra de implementação, não só de texto
- [x] Estrutura de projeto cobre DSP, UI, testes e CI
- [ ] Aprovado para gerar `tasks.md` executável (pendente confirmação do usuário)
