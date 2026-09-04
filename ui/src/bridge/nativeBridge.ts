// Contrato JS<->C++ definido em specs/001-nikolayevsk-dark-dsp/research.md (secao 4) e
// plan.md (ParameterLayout). Mantenha os IDs de parametro em sincronia com
// src/plugin/ParameterLayout.h -- nao ha checagem automatica entre os dois lados ainda.
//
// A ponte de baixo nivel (window.__JUCE__, getNativeFunction) vem do pacote oficial
// "juce-framework-frontend", vendorizado em ui/src/juce-frontend/ a partir do JUCE
// efetivamente baixado via FetchContent (build/_deps/juce-src/modules/juce_gui_extra/native/
// javascript) -- ver comentario de proveniencia la dentro. Isso substitui uma tentativa
// anterior (incorreta) de usar window.__JUCE__.getNativeFunction diretamente, que nao existe
// nessa API real: getNativeFunction e uma funcao exportada pelo pacote, nao um metodo do
// window.__JUCE__.

import { getNativeFunction as rawGetNativeFunction } from "../juce-frontend/index.js";

const getNativeFunction = rawGetNativeFunction as (
  name: string,
) => (...args: unknown[]) => Promise<unknown>;

export type AmpMode = "clean" | "rhythm" | "lead";

export interface PluginParams {
  ampMode: AmpMode;
  gain: number;
  bass: number;
  mid: number;
  treble: number;
  presence: number;
  master: number;
  tsDrive: number;
  tsTone: number;
  tsLevel: number;
  tsBypass: boolean;
}

export interface IrState {
  loaded: boolean;
  name: string;
  isDefault: boolean;
}

type ParamListener = (params: Partial<PluginParams>) => void;
type IrListener = (ir: IrState) => void;

declare global {
  interface Window {
    __JUCE__?: {
      initialisationData?: { __juce__platform?: string[] };
      backend?: {
        addEventListener: (eventId: string, fn: (payload: unknown) => void) => unknown;
      };
    };
  }
}

const DEFAULT_PARAMS: PluginParams = {
  ampMode: "clean",
  gain: 0.4,
  bass: 0.5,
  mid: 0.5,
  treble: 0.5,
  presence: 0.5,
  master: 0.7,
  tsDrive: 0.3,
  tsTone: 0.5,
  tsLevel: 0.7,
  tsBypass: true,
};

// check_native_interop.js (importado por juce-frontend/index.js) sempre define
// window.__JUCE__ com um polyfill inerte quando nao ha backend real -- entao "existe" nao
// serve mais como deteccao. __juce__platform so vem populado (nao-vazio) pelo backend nativo
// de verdade, e e o mesmo criterio usado internamente pelo proprio pacote da JUCE.
function isRunningInsideJuce(): boolean {
  const platform = window.__JUCE__?.initialisationData?.__juce__platform;
  return Array.isArray(platform) && platform.length > 0;
}

class NativeBridge {
  private paramListeners = new Set<ParamListener>();
  private irListeners = new Set<IrListener>();
  private mockState: PluginParams = { ...DEFAULT_PARAMS };
  private mockIr: IrState = { loaded: true, name: "default_cab (embutido)", isDefault: true };

  constructor() {
    if (isRunningInsideJuce()) {
      // NOTA (T022, ainda pendente no lado C++): para host-automation refletir na UI em
      // tempo real, o PluginEditor precisa chamar
      // WebBrowserComponent::emitEventIfBrowserIsVisible("paramChanged"/"irChanged", ...) --
      // isso ainda nao foi implementado, entao estes listeners ficam registrados mas nao
      // disparam ainda. Documentado tambem em tasks.md.
      window.__JUCE__!.backend?.addEventListener("paramChanged", (payload) => {
        this.paramListeners.forEach((listener) => listener(payload as Partial<PluginParams>));
      });
      window.__JUCE__!.backend?.addEventListener("irChanged", (payload) => {
        this.irListeners.forEach((listener) => listener(payload as IrState));
      });
    } else {
      // eslint-disable-next-line no-console
      console.warn(
        "[nativeBridge] Backend nativo do JUCE nao detectado -- rodando em modo mock de " +
          "navegador. Isso e esperado durante `npm run dev` fora do plugin.",
      );
    }
  }

  async setParam<K extends keyof PluginParams>(id: K, value: PluginParams[K]): Promise<void> {
    if (isRunningInsideJuce()) {
      await getNativeFunction("setParam")(id, value);
      return;
    }
    this.mockState[id] = value;
    this.paramListeners.forEach((listener) => listener({ [id]: value } as Partial<PluginParams>));
  }

  async getState(): Promise<PluginParams> {
    if (isRunningInsideJuce()) {
      const result = await getNativeFunction("getState")();
      return result as PluginParams;
    }
    return { ...this.mockState };
  }

  async loadIr(filePath: string): Promise<IrState> {
    if (isRunningInsideJuce()) {
      const result = await getNativeFunction("loadIR")(filePath);
      return result as IrState;
    }
    this.mockIr = { loaded: true, name: filePath.split("/").pop() ?? filePath, isDefault: false };
    this.irListeners.forEach((listener) => listener(this.mockIr));
    return this.mockIr;
  }

  // O picker de arquivo do host (juce::FileChooser) roda inteiramente no lado nativo --
  // a UI so dispara e recebe o resultado (FR-006). Em modo mock de navegador nao ha acesso
  // a filesystem real, entao simulamos com um nome fixo so para exercitar o fluxo visual.
  async browseForIr(): Promise<IrState> {
    if (isRunningInsideJuce()) {
      const result = await getNativeFunction("browseForIR")();
      return result as IrState;
    }
    return this.loadIr("meu_cabinet_custom.wav");
  }

  onParamChanged(listener: ParamListener): () => void {
    this.paramListeners.add(listener);
    return () => this.paramListeners.delete(listener);
  }

  onIrChanged(listener: IrListener): () => void {
    this.irListeners.add(listener);
    return () => this.irListeners.delete(listener);
  }
}

export const nativeBridge = new NativeBridge();
