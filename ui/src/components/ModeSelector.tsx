import type { AmpMode } from "../bridge/nativeBridge";

const MODES: { id: AmpMode; label: string }[] = [
  { id: "clean", label: "Clean" },
  { id: "rhythm", label: "Rhythm" },
  { id: "lead", label: "Lead" },
];

interface ModeSelectorProps {
  mode: AmpMode;
  onChange: (mode: AmpMode) => void;
}

// FR-001/FR-002/FR-010. Labels sao os mesmos usados publicamente ("Clean"/"Rhythm"/"Lead") --
// os identificadores internos de voicing (BRIT_HI/CALI_HI, ver plan.md) nunca aparecem aqui.
export function ModeSelector({ mode, onChange }: ModeSelectorProps) {
  return (
    <div className="modes">
      {MODES.map(({ id, label }) => (
        <button
          key={id}
          type="button"
          className={`mode-tab${id === mode ? " active" : ""}`}
          onClick={() => onChange(id)}
        >
          {label}
        </button>
      ))}
    </div>
  );
}
