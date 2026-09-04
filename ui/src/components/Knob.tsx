import { useCallback, useRef } from "react";

interface KnobProps {
  label: string;
  value: number;
  onChange: (value: number) => void;
  disabled?: boolean;
}

const START_DEG = -132;
const END_DEG = 132;
const DRAG_SENSITIVITY = 0.006;

// Knob giratorio de verdade (arraste vertical, como a maioria dos plugins de audio) --
// substitui o placeholder de slider do scaffold inicial, seguindo a identidade visual
// aprovada com o usuario (FR-V01/V02).
export function Knob({ label, value, onChange, disabled }: KnobProps) {
  const dragStart = useRef<{ clientY: number; value: number } | null>(null);

  const handlePointerDown = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      if (disabled) return;
      event.currentTarget.setPointerCapture(event.pointerId);
      dragStart.current = { clientY: event.clientY, value };
    },
    [disabled, value],
  );

  const handlePointerMove = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      if (!dragStart.current) return;
      const deltaY = dragStart.current.clientY - event.clientY;
      const next = Math.min(1, Math.max(0, dragStart.current.value + deltaY * DRAG_SENSITIVITY));
      onChange(next);
    },
    [onChange],
  );

  const handlePointerUp = useCallback(() => {
    dragStart.current = null;
  }, []);

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

  const dialDeg = START_DEG + (END_DEG - START_DEG) * value;
  const displayValue = Math.round(value * 10);

  return (
    <div className={`knob-unit${disabled ? " disabled" : ""}`}>
      <div
        className="knob-face"
        role="slider"
        aria-label={label}
        aria-valuemin={0}
        aria-valuemax={10}
        aria-valuenow={displayValue}
        aria-disabled={disabled}
        tabIndex={disabled ? -1 : 0}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onKeyDown={handleKeyDown}
      >
        <div className="knob-ticks">
          {Array.from({ length: 11 }, (_, n) => {
            const deg = START_DEG + (END_DEG - START_DEG) * (n / 10);
            return (
              <div key={n} className="tick" style={{ transform: `rotate(${deg}deg)` }}>
                <span style={{ transform: `translateX(-50%) rotate(${-deg}deg)` }}>{n}</span>
              </div>
            );
          })}
        </div>
        <div
          className="knob-dial"
          style={{ transform: `translate(-50%, -50%) rotate(${dialDeg}deg)` }}
        />
      </div>
      <span className="knob-label">{label}</span>
    </div>
  );
}
