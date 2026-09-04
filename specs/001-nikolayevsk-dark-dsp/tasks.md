# Tasks: Nikolayevsk DARK DSP

**Input**: `plan.md`, `research.md` (mesma pasta)
**Convenção**: `[P]` = paralelizável (sem dependência de outra task não concluída na mesma fase).
Tasks sem `[P]` dentro da mesma fase são sequenciais.

## Fase 0 — Setup do repositório

- [ ] **T001** Criar `CMakeLists.txt` raiz com `FetchContent` do JUCE 8 e subdiretório `src/`.
- [ ] **T002** [P] Criar `.gitignore` (build/, node_modules/, .vs/, *.user, dist/).
- [ ] **T003** [P] Criar `.github/workflows/ci.yml` com matrix `windows-latest`/`macos-latest`,
      build do VST3 + `pluginval` em modo estrito.
- [ ] **T004** [P] Inicializar app React+TS em `ui/` com Vite (`npm create vite@latest -- --template react-ts`).
- [ ] **T005** Definir `src/plugin/ParameterLayout.h` com o `APVTS::ParameterLayout` completo
      (todos os parâmetros de FR-004/FR-007: pedal Drive/Tone/Level/Bypass; amp Gain/Bass/Mid/
      Treble/Presence/Master; AmpMode enum Clean/Rhythm/Lead; IR slot state).
      **Bloqueia todas as tasks de DSP e UI abaixo** — é o contrato entre C++ e React.

## Fase 1 — DSP Core (test-first: escrever teste Catch2 antes da implementação de cada estágio)

- [ ] **T010** [P] Teste Catch2 + implementação de `TubeScreamerStage` (waveshaper assimétrico +
      filtro de tone), validando: bypass = bit-perfeito; drive=0 ≈ passagem quase linear;
      drive=1 produz clipping visível na forma de onda de teste.
- [ ] **T011** [P] Teste Catch2 + implementação de `ToneStack` (filtros Bass/Mid/Treble/
      Presence), validando resposta em frequência esperada em cada banda via FFT do sinal de
      teste.
- [ ] **T012** Teste Catch2 + implementação de `PreampStage` para os 3 modos (`BRIT_HI`,
      `CALI_HI` conforme guardrail de naming do `plan.md`, + modo Clean de baixo ganho),
      validando diferença objetiva de espectro/ganho entre os 3 modos com o mesmo sinal de
      entrada. **Depende de T005.**
- [ ] **T013** [P] Teste Catch2 + implementação de `PowerAmpSaturation`.
- [ ] **T014** Teste Catch2 + implementação de `CabinetLoader` (wrapper de
      `juce::dsp::Convolution`): carregar IR default embutido, carregar IR externo válido,
      rejeitar IR inválido com fallback para o default sem crash (edge case da spec).
      **Depende de** obtenção do arquivo de IR default (Open Question 1 do `plan.md` — ver
      T090 abaixo).
- [ ] **T015** Implementar `AmpChain` orquestrando a ordem completa: InputTrim → NoiseGate →
      TubeScreamerStage → PreampStage → ToneStack → PowerAmpSaturation → CabinetLoader →
      OutputTrim. **Depende de T010–T014.**
- [ ] **T016** Aplicar `juce::SmoothedValue` em todo parâmetro que afeta a cadeia em tempo real
      (endereça FR-012 — sem estalos em automação/troca de modo). **Depende de T015.**
- [ ] **T017** Teste de estabilidade: trocar sample rate e buffer size em runtime sem crash/
      estalo (simulação via harness de teste, antes de validar com `pluginval` real).

## Fase 2 — Plugin Shell (JUCE AudioProcessor/Editor)

- [ ] **T020** Implementar `PluginProcessor` conectando APVTS (T005) ao `AmpChain` (T015).
      **Depende de T005, T015.**
- [ ] **T021** Implementar `PluginEditor` hospedando `juce::WebBrowserComponent` com o bundle
      React (placeholder inicial, antes da UI real) servido via resource provider (não
      `file://`, conforme `research.md` seção 4).
- [ ] **T022** Implementar ponte nativa JS↔C++: `setParam`/`getState`/`loadIR` (JS→C++) e
      evento `paramChanged` (C++→JS) conforme contrato JSON de `research.md`. **Depende de
      T020, T021.**
- [ ] **T023** Validar que o áudio continua processando corretamente se o WebView falhar ao
      carregar (edge case da spec — UI nunca bloqueia o motor de áudio).

## Fase 3 — UI React/TS (paralelizável com Fase 1, depende só de T005 + T004)

- [ ] **T030** [P] Implementar `ui/src/bridge/nativeBridge.ts` tipado sobre o contrato JSON
      (mock local para desenvolver sem o host JUCE rodando, via `npm run dev` no browser).
- [ ] **T031** [P] Implementar tokens de tema gótico/rústico (`ui/src/theme/`): paleta de cor,
      tipografia, texturas — conforme FR-V01. Espaço reservado para assets da Haruna
      (FR-V03), sem placeholder gerado por IA marcado como "final".
- [ ] **T032** [P] Componente `ModeSelector` (Clean/Rhythm/Lead) com labels internos que
      respeitam o guardrail de naming (nunca expor "BRIT_HI"/"CALI_HI" nem termos de marca —
      usar rótulos próprios definidos em copy, ex. "Clean" / "Rhythm" / "Lead" como já são).
- [ ] **T033** [P] Componente `AmpHead` (Gain/Bass/Mid/Treble/Presence/Master) com knobs.
- [ ] **T034** [P] Componente `TubeScreamerPedal` (Drive/Tone/Level + footswitch visual).
- [ ] **T035** [P] Componente `Cabinet` (representação de caixa + mic + slot de carregar IR
      custom, FR-006) com feedback de erro visível para IR inválido (edge case da spec).
- [ ] **T036** Integrar T030–T035 na tela principal, conectando estado real ao `nativeBridge`.
      **Depende de T030–T035, e de T022 para teste end-to-end real dentro do plugin.**

## Fase 4 — Integração e Presets

- [ ] **T040** Implementar salvar/restaurar estado completo via `getStateInformation`/
      `setStateInformation` do APVTS, incluindo referência ao IR carregado (FR-008).
      **Depende de T020, T014.**
- [ ] **T041** Criar preset factory default "plug-and-play" com IR embutido ativo e valores de
      modo/pedal que já soam bem sem edição (FR-009). **Depende de T040, T090.**
- [ ] **T042** Teste cross-platform de preset: salvar em uma plataforma, validar
      bit-a-bit/valor-a-valor o carregamento (pode ser simulado localmente serializando e
      restaurando, antes de ter acesso físico às duas plataformas).

## Fase 5 — Validação e CI

- [ ] **T050** Rodar `pluginval` estrito localmente (macOS e, se disponível, Windows/VM) e
      corrigir todos os findings antes de considerar a Fase 1–4 "done".
- [ ] **T051** Benchmark de CPU por instância (NFR-001) com profiling real (não estimativa) —
      documentar resultado em `research.md` ou anexo de benchmark.
- [ ] **T052** Sessão de escuta A/B por Nikolayevsk contra referências reais (fecha o checklist
      pendente de `spec.md`) — ajustar `PreampStage`/`ToneStack` conforme feedback.

## Itens bloqueadores externos (não são tasks de código)

- [ ] **T090** Obter/capturar arquivo de IR de cabinet default com licença compatível com
      distribuição gratuita/open source (Open Question 1 do `plan.md`). **Bloqueia T014, T041.**
- [ ] **T091** Receber assets de identidade visual final da Haruna Lamberti (Open Question 2 do
      `plan.md`) — até lá, T031 usa placeholders explicitamente marcados como temporários.

## Ordem sugerida de execução (dependências resumidas)

```
T001–T004 [P] → T005 ──┬─→ T010–T013 [P] → T012(dep T005) → T015 → T016 → T017
                        │                                      ↑
                        │                            T014 (dep T090) ─┘
                        └─→ T030–T035 [P] (paralelo total com Fase 1)
T020 (dep T005,T015) → T021 → T022 → T023
T036 (dep T030-T035, T022)
T040 (dep T020,T014) → T041 (dep T090) → T042
T050, T051, T052 (fecham o ciclo)
```

## Review & Acceptance Checklist

- [ ] Toda task tem rastreabilidade a um FR/NFR de `spec.md` ou decisão de `plan.md`
- [ ] Bloqueadores externos (IR real, assets de arte) isolados e não escondidos dentro de tasks
      de código
- [ ] Aprovado para começar a implementação (pendente confirmação do usuário)
