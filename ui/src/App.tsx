import { useEffect, useState } from "react";
import { AmpHead } from "./components/AmpHead";
import { TubeScreamerPedal } from "./components/TubeScreamerPedal";
import { Cabinet } from "./components/Cabinet";
import { nativeBridge, type IrState, type PluginParams } from "./bridge/nativeBridge";
import "./theme/amp.css";

const INITIAL_IR: IrState = { loaded: true, name: "carregando...", isDefault: true };

export default function App() {
  const [params, setParams] = useState<PluginParams | null>(null);
  const [ir, setIr] = useState<IrState>(INITIAL_IR);
  const [irError, setIrError] = useState<string | null>(null);

  useEffect(() => {
    nativeBridge.getState().then(setParams);
    const unsubParams = nativeBridge.onParamChanged((partial) =>
      setParams((prev) => (prev ? { ...prev, ...partial } : prev)),
    );
    const unsubIr = nativeBridge.onIrChanged(setIr);
    return () => {
      unsubParams();
      unsubIr();
    };
  }, []);

  if (!params) {
    return <div className="dark-dsp-app dark-dsp-footer">Conectando ao motor de audio...</div>;
  }

  const setParam = <K extends keyof PluginParams>(id: K, value: PluginParams[K]) => {
    setParams((prev) => (prev ? { ...prev, [id]: value } : prev));
    nativeBridge.setParam(id, value);
  };

  const handleBrowseForIr = async () => {
    setIrError(null);
    try {
      const result = await nativeBridge.browseForIr();
      setIr(result);
      return result;
    } catch {
      setIrError("Nao foi possivel carregar esse IR -- mantendo o cabinet padrao.");
      return ir;
    }
  };

  return (
    <div className="dark-dsp-app">
      <div className="rig">
        <AmpHead
          params={params}
          mode={params.ampMode}
          onModeChange={(mode) => setParam("ampMode", mode)}
          onParamChange={(id, value) => setParam(id, value)}
        />
        <div className="lower">
          <TubeScreamerPedal
            params={params}
            bypass={params.tsBypass}
            onParamChange={(id, value) => setParam(id, value)}
            onBypassToggle={() => setParam("tsBypass", !params.tsBypass)}
          />
          <Cabinet ir={ir} onBrowseForIr={handleBrowseForIr} error={irError} />
        </div>
        <footer className="dark-dsp-footer">NIKOLAYEVSK DARK DSP — SIGNATURE AMP SIMULATOR</footer>
      </div>
    </div>
  );
}
