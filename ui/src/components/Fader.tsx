import { useCallback, useRef } from "react";

interface FaderProps {
  label: string;
  value: number;
  onChange: (value: number) => void;
  disabled?: boolean;
}

// Fader vertical estilo pedal (arraste direto na trilha, valor proporcional a posicao) --
// usado pelos controles Drive/Tone/Level do TubeScreamerPedal.
export function Fader({ label, value, onChange, disabled }: FaderProps) {
  const trackRef = useRef<HTMLDivElement>(null);

  const setFromClientY = useCallback(
    (clientY: number) => {
      const track = trackRef.current;
      if (!track) return;
      const rect = track.getBoundingClientRect();
      const ratio = 1 - (clientY - rect.top) / rect.height;
      onChange(Math.min(1, Math.max(0, ratio)));
    },
    [onChange],
  );

  const handlePointerDown = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      if (disabled) return;
      event.currentTarget.setPointerCapture(event.pointerId);
      setFromClientY(event.clientY);
    },
    [disabled, setFromClientY],
  );

  const handlePointerMove = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      if (event.buttons !== 1) return;
      setFromClientY(event.clientY);
    },
    [setFromClientY],
  );

  const handleKeyDown = useCallback(
    (event: React.KeyboardEvent<HTMLDivElement>) => {
      if (disabled) return;
      if (event.key === "ArrowUp" || event.key === "ArrowRight") {
        onChange(Math.min(1, value + 0.02));
        event.preventDefault();
      } else if (event.key === "ArrowDown" || event.key === "ArrowLeft") {
        onChange(Math.max(0, value - 0.02));
        event.preventDefault();
      }
    },
    [disabled, value, onChange],
  );

  return (
    <div className="fader-unit">
      <div
        ref={trackRef}
        className="fader-track"
        role="slider"
        aria-label={label}
        aria-valuemin={0}
        aria-valuemax={1}
        aria-valuenow={value}
        aria-disabled={disabled}
        tabIndex={disabled ? -1 : 0}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onKeyDown={handleKeyDown}
      >
        <div className="fader-cap" style={{ bottom: `calc(${value * 100}% - 6px)` }} />
      </div>
      <span className="fader-label">{label}</span>
    </div>
  );
}
