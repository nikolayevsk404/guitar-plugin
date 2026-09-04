import { Fader } from "./Fader";
import type { PluginParams } from "../bridge/nativeBridge";

type PedalParamKey = "tsDrive" | "tsTone" | "tsLevel";

interface TubeScreamerPedalProps {
  params: Pick<PluginParams, PedalParamKey>;
  bypass: boolean;
  onParamChange: (id: PedalParamKey, value: number) => void;
  onBypassToggle: () => void;
}

const FADERS: { id: PedalParamKey; label: string }[] = [
  { id: "tsDrive", label: "Drive" },
  { id: "tsTone", label: "Tone" },
  { id: "tsLevel", label: "Level" },
];

// Pedal overdrive (FR-004/FR-010). Bypass e hard bypass no DSP (research.md secao 2) --
// aqui e so o estado visual do footswitch (aceso = pedal ligado).
export function TubeScreamerPedal({ params, bypass, onParamChange, onBypassToggle }: TubeScreamerPedalProps) {
  return (
    <section className="panel leather pedal">
      <span className="panel-title">Overdrive</span>
      <div className="pedal-sliders">
        {FADERS.map(({ id, label }) => (
          <Fader
            key={id}
            label={label}
            value={params[id]}
            disabled={bypass}
            onChange={(v) => onParamChange(id, v)}
          />
        ))}
      </div>
      <button
        type="button"
        className={`footswitch${bypass ? " off" : ""}`}
        aria-pressed={!bypass}
        title={bypass ? "Pedal desligado (bypass)" : "Pedal ligado"}
        onClick={onBypassToggle}
      />
    </section>
  );
}
