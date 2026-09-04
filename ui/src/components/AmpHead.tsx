import { Knob } from "./Knob";
import { ModeSelector } from "./ModeSelector";
import type { AmpMode, PluginParams } from "../bridge/nativeBridge";

type AmpParamKey = "gain" | "bass" | "mid" | "treble" | "presence" | "master";

interface AmpHeadProps {
  params: Pick<PluginParams, AmpParamKey>;
  mode: AmpMode;
  onModeChange: (mode: AmpMode) => void;
  onParamChange: (id: AmpParamKey, value: number) => void;
}

const KNOBS: { id: AmpParamKey; label: string }[] = [
  { id: "gain", label: "Gain" },
  { id: "bass", label: "Bass" },
  { id: "mid", label: "Mid" },
  { id: "treble", label: "Treble" },
  { id: "presence", label: "Presence" },
  { id: "master", label: "Master" },
];

// Cabecote (FR-007/FR-010). Os dois ".arch" ficam vazios de proposito -- reservados pra uma
// imagem de bode real (FR-V03), ainda nao fornecida pela direcao de arte da Haruna.
// POWER/STANDBY sao decorativos (sem parametro correspondente no APVTS) -- so reforcam a
// sensacao de hardware fisico (FR-V04).
export function AmpHead({ params, mode, onModeChange, onParamChange }: AmpHeadProps) {
  return (
    <section className="head leather">
      <div className="strap" />

      <div className="head-top">
        <span className="brand">Nikolayevsk · DARK DSP · Signature Amp Simulator</span>
        <ModeSelector mode={mode} onChange={onModeChange} />
      </div>

      <div className="plate-row">
        <div className="arch" />

        <div className="nameplate">
          <div className="wordmark">
            Nikolayevsk <span>·</span> DARK DSP
          </div>
        </div>

        <div className="arch" />
      </div>

      <div className="control-rail">
        <div className="switch-group">
          <div className="switch-unit">
            <div className="switch">
              <div className="led" />
            </div>
            <span className="switch-label">POWER</span>
          </div>
          <div className="switch-unit">
            <div className="switch off">
              <div className="led" />
            </div>
            <span className="switch-label">STANDBY</span>
          </div>
        </div>

        <div className="knobs">
          {KNOBS.map(({ id, label }) => (
            <Knob key={id} label={label} value={params[id]} onChange={(v) => onParamChange(id, v)} />
          ))}
        </div>

        <div className="input-jack">
          <div className="jack-ring" />
          <span className="switch-label">INPUT</span>
        </div>
      </div>
    </section>
  );
}
