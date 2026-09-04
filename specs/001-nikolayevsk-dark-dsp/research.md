# Research Notes: Nikolayevsk DARK DSP

Estas notas fundamentam as decisões de `plan.md`. Não é literatura acadêmica — é o racional
técnico mínimo para justificar por que cada estágio de DSP existe e por que é modelado assim.

## 1. Por que "fusão de voicing" e não "clone de circuito"

Emulação de circuito ciclo-exato (SPICE/WDF de um schematic real) exige o schematic real, é
caro computacionalmente, e — mais importante — reproduz *um* amp específico, não uma fusão.
Como o objetivo é uma sonoridade signature (nova, não uma réplica), a abordagem correta é
**behavioral modeling**: waveshaping + filtros que reproduzem o *comportamento perceptível*
(resposta em frequência, forma de compressão/saturação por estágio) descrito na tabela
"Voicing Target" da spec, não a topologia elétrica exata. Isso também elimina o risco de
trademark tratado em `plan.md` — não há "clone" para reivindicar semelhança de nome.

### Características que diferenciam os dois arquétipos (uso interno, sem nomear marcas em código)

- **Britânico de alto gain**: geralmente 2 estágios de gain em cascata, tone stack tipo
  Baxandall pós-estágio de gain, presença como realce de agudos no loop de realimentação do
  estágio de potência. Resultado perceptível: médios "cortantes", resposta que reage bastante
  à dinâmica da palhetada (breakup sensível ao ataque).
- **Americano de gain em cascata**: 3+ estágios de gain, tone stack por vezes com controle de
  "contour" (scoop de médio) antes do master, filtro passa-alta mais agressivo antes do
  primeiro estágio de gain (reduz flabbiness em afinações graves/palm mute). Resultado
  perceptível: mais ganho disponível, graves mais controlados, sustain mais liso.
- **Implicação de implementação**: o `PreambStage` de cada modo não deve ser "um waveshaper
  genérico com gain diferente" — precisa variar (a) posição do filtro passa-alta pré-gain
  (mais agressivo em Rhythm/Lead), (b) número/ordem de estágios de saturação em cascata, e
  (c) a curva do tone stack (mid scoop disponível apenas em Rhythm/Lead).

## 2. Modelagem do pedal Tube Screamer-like

Circuito real: op-amp em configuração de soft-clipping com diodos no loop de realimentação,
filtro passa-baixa suave no estágio de tone, resposta de médios elevada característica (o
"médio do 808" citado na cultura de guitarristas). Para behavioral modeling:

- **Drive** → controla ganho de entrada no waveshaper (curva assimétrica leve — diodos reais
  não são perfeitamente simétricos, e essa assimetria é parte do caráter do pedal).
- **Tone** → filtro shelving que corta/realça agudos pós-clipping.
- **Level** → gain de saída linear, não afeta a curva de saturação.
- **Bypass** deve ser *hard bypass* (roteamento que pula o estágio), não "amount = 0", porque
  mesmo com drive zerado um circuito real ainda filtra o sinal ligeiramente — mas para efeito
  de plugin, bypass = sinal bit-perfeito é o comportamento esperado por qualquer guitarrista
  (é o padrão de mercado em pedais virtuais).

## 3. Convolução de Impulse Response (cabinet + microfone)

`juce::dsp::Convolution` cobre o caso de uso sem reinventar FFT/overlap-add. Pontos de atenção
de implementação (não de decisão, já que a lib está decidida em `plan.md`):

- IRs precisam bater o sample rate do host — a classe já reamostra internamente, mas isso tem
  custo de CPU perceptível em troca de sample rate em tempo real; por isso o edge case da spec
  sobre troca de sample rate exige realocação assíncrona (não travar o `processBlock`).
- Tamanho típico de IR de cabinet: 100–500ms. Em buffers pequenos (ex. 64 samples @ 96kHz),
  isso implica processamento particionado (a própria classe do JUCE já faz isso
  internamente) — não é necessário implementar particionamento manual.
- IR default embutido (plug-and-play, FR-009) precisa ser um arquivo real capturado com
  microfone ou obtido de fonte com licença compatível com distribuição — **não pode ser
  sintetizado por código** sem taxa de realismo aceitável; isso está registrado como Open
  Question em `plan.md`.

## 4. Integração WebView (React/TS) dentro do JUCE

O JUCE 8 tem `juce::WebBrowserComponent` com dois modos relevantes:

- **Modo simples**: só navega para uma URL/HTML local — insuficiente, porque precisamos de
  comunicação bidirecional tempo real (parâmetros mudando por automação do host devem
  refletir na UI, e a UI precisa poder setar parâmetro e disparar load de IR).
- **Modo com native function bindings** (`WebBrowserComponent::Options().withNativeFunction(...)`
  / `withEventListener(...)` dependendo da versão exata do JUCE 8): permite registrar funções
  C++ chamáveis via JS (`window.__JUCE__...` conforme a convenção do JUCE) e emitir eventos do
  C++ para o JS. Esse é o modo que o projeto usa.
- **Empacotamento do bundle React**: o `dist/` gerado pelo Vite é convertido em `BinaryData`
  (recurso embutido no binário do plugin, via o mecanismo de binary resources do JUCE/CMake),
  e o `WebBrowserComponent` serve esse conteúdo via `juce::WebBrowserComponent::Resource`
  (resource provider), não via `file://` nem servidor HTTP — evita problema de path/permissão
  em instalação final e evita abrir uma porta local (mais seguro, sem superfície de rede).
- **Contrato de mensagens**: JSON simples nos dois sentidos —
  `{"type": "setParam", "id": "gain", "value": 0.6}` (JS→C++) e
  `{"type": "paramChanged", "id": "gain", "value": 0.6}` (C++→JS, para refletir automação de
  host). Esse contrato deve ser tipado no lado TS (`ui/src/bridge/nativeBridge.ts`) para
  aproveitar o TypeScript do usuário e evitar bugs de string solta.

## 5. Por que CMake + FetchContent em vez de Projucer

Projucer gera projetos de IDE (Xcode/.sln) que não são reproduzíveis em CI headless sem
esforço extra e tende a divergir entre máquinas (arquivo gerado versionado ou não). CMake com
`FetchContent` para o JUCE deixa o build declarativo, idêntico em Windows/macOS/CI, e permite
automatizar o passo de build do app React (via `execute_process`/custom target) dentro do
mesmo `cmake --build`, atendendo ao requisito de "plug and play" também para quem vai
**compilar** o projeto (contribuidores open source), não só para quem usa o plugin final.

## 6. Riscos conhecidos a monitorar durante implementação

- **CPU do WebView**: WebViews consomem mais memória/CPU que uma UI JUCE nativa (Components).
  Para um plugin de guitarra usado em múltiplas instâncias numa sessão, isso é um risco real
  para NFR-001. Mitigação planejada: UI só re-renderiza em mudança de estado (não em cada
  bloco de áudio) e VU-meter/metering usa taxa de atualização baixa (ex. 30fps), não a taxa de
  callback de áudio.
- **Determinismo de build cross-platform**: dependências nativas do JUCE em Linux/macOS podem
  exigir pacotes de sistema (ex. bibliotecas gráficas no Linux — fora de escopo, mas documentar
  se algum contribuidor tentar buildar em Linux mesmo sem ser alvo oficial).
