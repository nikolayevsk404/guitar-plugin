# Nikolayevsk DARK DSP

<img width="1279" height="775" alt="image" src="https://github.com/user-attachments/assets/0c33ade7-5924-496f-8b9e-d82472984886" />

Plugin de guitarra VST3, **gratuito e open source (GPLv3)**, signature do
[Nikolayevsk](https://github.com/nikolayevsk404) — guitarrista brasileiro de progressive power
metal. Fusão de voicing entre um amp britânico de alto gain e um amp americano de gain em
cascata, com pedal overdrive, cabinet/IR loader e identidade visual gótica/rústica.

> Projeto em desenvolvimento inicial (alpha), seguindo Spec-Driven Development — a
> implementação segue os documentos em `specs/001-nikolayevsk-dark-dsp/`. Já compila e carrega
> como VST3 (validado no Reaper/macOS); ainda faltam o IR de cabinet real e a identidade
> visual final (ver `tasks.md`, itens T090/T091).

## Cadeia de sinal

```
Guitarra → Pedal (Tube Screamer-like) → Cabeçote (Clean | Rhythm | Lead) → Caixa + Mic (IR) → DAW
```

## Documentação (Spec-Driven Development)

Todo o desenvolvimento parte de specs versionadas antes do código:

- [`spec.md`](specs/001-nikolayevsk-dark-dsp/spec.md) — o quê e por quê (requisitos,
  critérios de aceite, voicing target, identidade visual).
- [`plan.md`](specs/001-nikolayevsk-dark-dsp/plan.md) — como (arquitetura, stack, estrutura
  de projeto).
- [`research.md`](specs/001-nikolayevsk-dark-dsp/research.md) — racional técnico por trás de
  cada decisão de DSP e de integração UI↔áudio.
- [`tasks.md`](specs/001-nikolayevsk-dark-dsp/tasks.md) — quebra executável de tarefas.

## Stack

- **DSP / host de plugin**: C++20, [JUCE](https://juce.com/) 8.x, CMake
- **UI**: React 18 + TypeScript (Vite), renderizada dentro do `WebBrowserComponent` do JUCE
- **Formato**: VST3 (Windows 10+ e macOS 12+)
- **Testes**: Catch2 (DSP), `pluginval` (validação de plugin), GitHub Actions (CI multiplataforma)

## Licença

[GPLv3](LICENSE). Uso, modificação e redistribuição livres, inclusive comercial, desde que o
código-fonte de qualquer versão distribuída permaneça aberto sob a mesma licença.

## Identidade visual

Direção de arte por Haruna Lamberti. Estética gótica/rústica alinhada a prog/power metal —
detalhes em [`spec.md`](specs/001-nikolayevsk-dark-dsp/spec.md#identidade-visual-mandatory).

## Build

Pré-requisitos: [CMake](https://cmake.org/) 3.22+, um compilador C++20 (Xcode/clang no macOS,
MSVC no Windows), Node.js 20+.

**Windows apenas**: a UI roda em WebView2 (Edge/Chromium), que exige o SDK
`Microsoft.Web.WebView2` disponível em tempo de compilação. Baixe o `.nupkg` e aponte o CMake
para a pasta onde extraiu (mesma lógica usada em `.github/workflows/ci.yml`):

```powershell
mkdir webview2_packages\Microsoft.Web.WebView2.1.0.4078.44
curl.exe -L -o webview2.nupkg "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.4078.44"
tar -xf webview2.nupkg -C webview2_packages\Microsoft.Web.WebView2.1.0.4078.44
```

**Windows apenas**: passe sempre `-A x64` ao configurar -- sem isso o gerador do Visual
Studio assume Win32 (x86) por padrao e gera um `.vst3` que nenhuma DAW de 64 bits carrega
("nao foi projetado para ser executado no Windows ou contem um erro").

```powershell
cmake -B build -A x64 -DJUCE_WEBVIEW2_PACKAGE_LOCATION="%cd%\webview2_packages" ...
```

```bash
# 1. Build da UI (React/TS) -- precisa ser feito antes do CMake configurar
cd ui
npm install
npm run build
cd ..

# 2. Configure + build do plugin (baixa o JUCE e o Catch2 automaticamente via CMake FetchContent)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

O `.vst3` gerado é copiado automaticamente para a pasta de plugins VST3 do sistema
(`COPY_PLUGIN_AFTER_BUILD`). Builds de CI (Windows + macOS) ficam disponíveis para download em
**Actions → (run) → Artifacts** no GitHub, mesmo sem ter uma máquina Windows à mão.

```bash
# Testes de DSP (Catch2)
ctest --test-dir build -C Release --output-on-failure
```
