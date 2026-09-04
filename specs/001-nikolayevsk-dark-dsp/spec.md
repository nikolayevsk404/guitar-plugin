# Feature Specification: Nikolayevsk DARK DSP

**Feature Branch**: `001-nikolayevsk-dark-dsp`
**Created**: 2026-08-24
**Status**: Draft
**Author**: Guilherme Augusto (Nikolayevsk)

**Input (resumo do pedido original)**: Plugin VST3 de guitarra, open source e gratuito, signature do
Nikolayevsk. Fusão sonora entre um amp britânico de alto gain estilo Marshall JCM800 e um amp
americano estilo Mesa Boogie. Três modos — Clean, Rhythm, Lead. Cadeia completa: pedal
overdrive estilo Tube Screamer → cabeçote → caixa (cab) → microfone/IR loader. Identidade
visual gótica/rústica, alinhada a prog/power metal. Multiplataforma (Windows + macOS), VST3,
plug-and-play (zero configuração para sonar bem no preset default).

---

## User Scenarios & Testing *(mandatory)*

### Primary User Story
Um guitarrista de prog/power metal (o próprio Nikolayevsk ou qualquer usuário que baixe o
plugin) abre sua DAW, carrega o "Nikolayevsk DARK DSP" em uma pista de guitarra, seleciona o
modo **Lead**, toca sem tocar em mais nenhum parâmetro, e já ouve um tom de solo com sustain,
saturação e definição adequados para gravação ou prática — sem precisar entender amp
modeling, impulse responses ou tone stacks.

### Acceptance Scenarios

1. **Given** o plugin recém-instanciado com o preset default, **When** o usuário seleciona o
   modo **Clean**, **Then** o sinal sai com baixo ganho, dinâmica preservada e sem clipping
   audível em transientes de palhetada forte.
2. **Given** o modo **Rhythm** selecionado, **When** o usuário toca power chords palhetados,
   **Then** o sinal apresenta ganho médio-alto, resposta rápida (tight) na região de graves e
   separação de notas perceptível (sem "mush").
3. **Given** o modo **Lead** selecionado, **When** o usuário sustenta uma nota bendada em
   registro agudo, **Then** o sinal sustenta com saturação contínua e sem oscilação/ringing
   indesejado, com presença de médios altos suficiente para "cortar" no mix.
4. **Given** o pedal Tube Screamer virtual habilitado antes do amp, **When** o usuário aumenta o
   parâmetro Drive do pedal, **Then** o gain de entrada do amp aumenta e o timbre ganha o
   médio característico de overdrive a tubo, sem introduzir ruído digital perceptível.
5. **Given** um arquivo de Impulse Response (.wav) do usuário, **When** ele é carregado no slot
   de microfone/cabinet, **Then** o plugin substitui a simulação de cab interna por convolução
   com esse IR, sem glitches, cliques ou dropouts na troca.
6. **Given** o plugin aberto em uma DAW no Windows e o mesmo projeto aberto em uma DAW no
   macOS, **When** o preset é salvo e recarregado, **Then** o estado (modo, parâmetros, IR
   carregado) é recuperado de forma idêntica em ambas plataformas.
7. **Given** o usuário sem nenhum IR próprio, **When** ele abre o plugin por padrão,
   **Then** já existe pelo menos um IR de cabinet embutido funcionando (plug-and-play real,
   sem estado "mudo" por falta de arquivo externo).

### Edge Cases
- O que acontece se o usuário carregar um IR corrompido ou em sample rate incompatível?
  → Deve haver fallback para o IR interno + mensagem de erro clara na UI, sem crash.
- O que acontece se o host mudar o sample rate/buffer size em tempo real (ex: 44.1kHz → 96kHz)?
  → DSP deve realocar filtros/convolução sem estalos e sem travar a UI.
- O que acontece com automação de host nos parâmetros (ex: automação do "Gain" ao trocar de
  modo Clean→Lead no meio de uma nota sustentada)? → Transição deve ser suave (sem "pop"),
  idealmente com crossfade/smoothing de parâmetros.
- O que acontece se o WebView (UI React) falhar ao carregar (ex: recurso corrompido)?
  → O áudio deve continuar funcionando com os últimos parâmetros válidos; UI é camada
  desacoplada do motor de áudio, nunca bloqueante.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: O plugin DEVE oferecer três modos de amplificador — **Clean**, **Rhythm**, **Lead**
  — selecionáveis por um seletor de canal único (comportamento de footswitch de amp real).
- **FR-002**: Cada modo DEVE ter sua própria curva de ganho/saturação e tone stack, não sendo
  apenas o mesmo estágio com o "Gain" em posições diferentes.
- **FR-003**: O motor de amplificador DEVE soar como uma fusão perceptível entre um voicing
  britânico de alto gain (estilo Marshall JCM800) e um voicing americano (estilo Mesa Boogie) —
  ver seção "Voicing Target" abaixo para a definição objetiva dessa fusão.
- **FR-004**: O plugin DEVE incluir um pedal de overdrive virtual estilo Tube Screamer,
  posicionado antes do amp na cadeia de sinal, com parâmetros Drive, Tone e Level, e um
  interruptor on/off.
- **FR-005**: O plugin DEVE incluir um estágio de cabinet + microfone, com pelo menos um IR
  (impulse response) de cabinet embutido no binário (funciona sem nenhum arquivo externo).
- **FR-006**: O plugin DEVE permitir carregar arquivos de IR externos (.wav) do usuário no
  lugar do IR embutido, via seletor de arquivo na UI.
- **FR-007**: O plugin DEVE expor controles de amp equivalentes a um cabeçote real: Gain,
  Bass, Mid, Treble, Presence e Master Volume, com comportamento por modo (FR-002).
- **FR-008**: O plugin DEVE salvar e restaurar todo o estado (modo, parâmetros do pedal, do amp,
  caminho/estado do IR) através do mecanismo de presets do host (state save/restore da DAW).
- **FR-009**: O plugin DEVE ser plug-and-play: ao ser instanciado sem nenhuma configuração
  prévia do usuário, já deve produzir um tom utilizável (não silencioso, não cru/sem
  processamento) no preset default.
- **FR-010**: A interface DEVE representar visualmente um cabeçote de amp, uma caixa (cabinet)
  com indicação de microfone, e um pedal estilo stompbox, seguindo a identidade visual
  gótica/rústica prog/power metal (ver seção de Identidade Visual).
- **FR-011**: O plugin DEVE funcionar como VST3 em Windows 10+ e macOS 12+, em hosts padrão de
  mercado (Reaper, Cubase, Studio One, Ableton Live, Bitwig).
- **FR-012**: Mudanças de parâmetro (incluindo troca de modo Clean/Rhythm/Lead) NÃO DEVEM
  produzir estalos/cliques audíveis (parameter smoothing obrigatório).
- **FR-013**: O plugin DEVE ser distribuído com código-fonte aberto sob licença GPLv3,
  gratuito, sem tela de ativação/licenciamento nem telemetria.

### Non-Functional Requirements

- **NFR-001 (Performance)**: Uso de CPU por instância DEVE ficar dentro de uma faixa aceitável
  para uso em faixas múltiplas numa sessão real (meta: comparável a plugins de amp sim
  comerciais equivalentes — validar com profiling, não é um número arbitrário sem medição).
- **NFR-002 (Latência)**: O plugin NÃO DEVE introduzir latência de processamento perceptível
  em uso de monitoramento em tempo real (sem oversampling excessivo forçado por padrão; se
  oversampling for usado para qualidade da distorção, deve ser configurável).
- **NFR-003 (Estabilidade)**: Zero crashes em troca de sample rate, buffer size, bypass,
  automação de host e carregamento/troca de IR, validados via `pluginval` em modo estrito.
- **NFR-004 (Portabilidade)**: Mesmo binário de UI (React/TS) e mesma árvore de DSP (C++)
  devem compilar e se comportar identicamente em Windows e macOS — sem branches de
  comportamento sonoro por plataforma.
- **NFR-005 (Simplicidade de uso)**: Um usuário sem conhecimento de amp modeling deve
  conseguir obter um tom satisfatório usando só o seletor de modo, sem tocar em EQ.

### Voicing Target (definição objetiva da fusão sonora)

Esta seção existe para tirar "fusão Marshall JCM800 + Mesa Boogie" do campo subjetivo e
transformar em critério verificável durante o desenvolvimento (ver `research.md` para o
racional técnico de cada característica):

| Característica                        | Herança predominante        |
|---------------------------------------|------------------------------|
| Crunch de médios, breakup dinâmico ao ataque da palhetada | Britânico (JCM800) |
| Resposta de graves "tight"/controlada, sem flabbiness em palm mute | Americano (Mesa) |
| Presença/brilho cortante em agudos no modo Lead | Britânico (JCM800) |
| Ganho em cascata (múltiplos estágios) para Rhythm/Lead | Americano (Mesa) |
| Tone stack com mid scoop opcional (contour-like) no Rhythm | Americano (Mesa) |
| Clean com headroom alto e sino levemente compressivo | Híbrido (ambos) |

> ⚠️ **Nota legal/trademark**: "Marshall", "JCM800" e "Mesa Boogie" são marcas registradas de
> terceiros. Elas são usadas aqui **apenas como referência interna de design sonoro** (uso
> comum na comunidade de amp-modeling para descrever timbre-alvo). O produto final, sendo
> distribuído publicamente, **não deve** usar esses nomes em UI, marketing, nome de presets ou
> metadados do plugin — recomenda-se nomenclatura própria (ex.: "Brit Crunch" / "Cali Gain"
> como nomes de voicing internos). Isso será tratado como requisito em `plan.md`.

---

## Identidade Visual *(mandatory)*

- **FR-V01**: Estética gótica/rústica alinhada a prog/power metal — paleta escura (preto,
  grafite, vinho/bordô, dourado/latão envelhecido como acento), texturas de metal escovado,
  couro/madeira desgastada, tipografia com serifa afiada ou blackletter sutil (sem comprometer
  legibilidade de valores numéricos).
- **FR-V02**: A UI DEVE representar 3 elementos físicos reconhecíveis: **cabeçote** (topo,
  controles de Gain/EQ/Presence/Master + seletor de modo), **caixa/cabinet** (com grelha e
  indicação visual de microfone posicionado), **pedal** (stompbox Tube Screamer-like, footswitch
  clicável).
- **FR-V03**: Identidade visual deve ser compatível com o trabalho de Haruna Lamberti (direção
  de arte do projeto Nikolayevsk) — a UI deve ter espaço reservado para asset(s) de logo/arte
  fornecidos por ela (não gerado por IA como arte final de marca).
- **FR-V04**: Motion/feedback visual (LEDs de footswitch, VU-meter ou similar) deve reforçar a
  sensação de hardware físico, não de plugin "flat" genérico.

---

## Key Entities

- **Amp Mode**: Clean | Rhythm | Lead — define curva de ganho, tone stack ativo e faixa de
  parâmetros default.
- **Pedal (Tube Screamer-like)**: Drive, Tone, Level, Bypass — estágio pré-amp.
- **Cabinet/Mic (IR Loader)**: IR ativo (embutido ou custom), posição de mic simulada (se
  aplicável a múltiplos IRs embutidos), ganho de saída do estágio.
- **Preset**: Snapshot de Mode + parâmetros do Amp + parâmetros do Pedal + referência de IR.
- **Global Controls**: Input trim, Output/Master, Noise Gate (necessário em alto gain).

---

## Review & Acceptance Checklist

- [x] Requisitos escritos em linguagem de comportamento observável (testável por escuta/medição)
- [x] Sem detalhe de implementação nesta camada (fica em `plan.md`)
- [x] Ambiguidade "fusão de timbre" resolvida em critério objetivo (tabela Voicing Target)
- [x] Risco de trademark identificado e endereçado como requisito de naming
- [ ] Validado com escuta A/B por Nikolayevsk contra referências reais (pendente — Fase de QA sonoro)
- [ ] Aprovado para avançar para `plan.md` (pendente confirmação do usuário)
