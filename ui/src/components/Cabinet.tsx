import { useState } from "react";
import type { IrState } from "../bridge/nativeBridge";

interface CabinetProps {
  ir: IrState;
  onBrowseForIr: () => Promise<IrState>;
  error: string | null;
}

// Caixa + microfone + IR loader (FR-005/FR-006/FR-V02).
export function Cabinet({ ir, onBrowseForIr, error }: CabinetProps) {
  const [busy, setBusy] = useState(false);

  const handleBrowse = async () => {
    setBusy(true);
    try {
      await onBrowseForIr();
    } finally {
      setBusy(false);
    }
  };

  return (
    <section className="panel leather cabinet">
      <span className="panel-title">Caixa + Microfone (IR)</span>
      <div>
        <div className="cabinet-status">
          <b>{ir.name}</b> — {ir.loaded ? "carregado" : "carregando..."}
        </div>
        {ir.isDefault && <div className="cabinet-sub">4×12 · dinâmico próximo · padrão embutido</div>}
        {error && <div className="ir-error">{error}</div>}
      </div>
      <button type="button" className="btn-ghost" disabled={busy} onClick={handleBrowse}>
        {busy ? "Carregando..." : "Carregar IR…"}
      </button>
    </section>
  );
}
