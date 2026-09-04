#!/usr/bin/env python3
"""Gera um Impulse Response PLACEHOLDER de cabinet, um pouco mais convincente que ruido
branco puro: aplica uma cadeia de biquads (RBJ cookbook) num impulso unitario pra moldar o
espectro como uma caixa 4x12 tipica (corte de sub-graves, ressonancia do cone, presence peak
de microfone, rolloff de agudos), mais um punhado de micro-reflexoes e um envelope de
decaimento em dois estagios.

Isso AINDA NAO substitui T090 (specs/001-nikolayevsk-dark-dsp/tasks.md): um IR real,
capturado ou com licenca compativel, precisa entrar antes de qualquer release publico. Mas
soa como "caixa+mic", nao como ruido cru -- resolve a reclamacao de "esta saindo direto do
cabecote sem caixa".
"""

import math
import random
import struct
import wave

SAMPLE_RATE = 44100
DURATION_S = 0.15
OUT_PATH = "assets/ir/default_cab_PLACEHOLDER.wav"


class Biquad:
    def __init__(self, b0, b1, b2, a1, a2):
        self.b0, self.b1, self.b2, self.a1, self.a2 = b0, b1, b2, a1, a2
        self.x1 = self.x2 = self.y1 = self.y2 = 0.0

    def process(self, x):
        y = self.b0 * x + self.b1 * self.x1 + self.b2 * self.x2 - self.a1 * self.y1 - self.a2 * self.y2
        self.x2, self.x1 = self.x1, x
        self.y2, self.y1 = self.y1, y
        return y

    @staticmethod
    def highpass(freq, q, sr):
        w0 = 2 * math.pi * freq / sr
        alpha = math.sin(w0) / (2 * q)
        cosw0 = math.cos(w0)
        b0 = (1 + cosw0) / 2
        b1 = -(1 + cosw0)
        b2 = (1 + cosw0) / 2
        a0 = 1 + alpha
        a1 = -2 * cosw0
        a2 = 1 - alpha
        return Biquad(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0)

    @staticmethod
    def lowpass(freq, q, sr):
        w0 = 2 * math.pi * freq / sr
        alpha = math.sin(w0) / (2 * q)
        cosw0 = math.cos(w0)
        b0 = (1 - cosw0) / 2
        b1 = 1 - cosw0
        b2 = (1 - cosw0) / 2
        a0 = 1 + alpha
        a1 = -2 * cosw0
        a2 = 1 - alpha
        return Biquad(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0)

    @staticmethod
    def peaking(freq, q, gain_db, sr):
        a = 10 ** (gain_db / 40)
        w0 = 2 * math.pi * freq / sr
        alpha = math.sin(w0) / (2 * q)
        cosw0 = math.cos(w0)
        b0 = 1 + alpha * a
        b1 = -2 * cosw0
        b2 = 1 - alpha * a
        a0 = 1 + alpha / a
        a1 = -2 * cosw0
        a2 = 1 - alpha / a
        return Biquad(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0)


def build_cabinet_filter_chain(sr):
    return [
        Biquad.highpass(85.0, 0.707, sr),      # cab nao reproduz sub-graves
        Biquad.peaking(105.0, 2.2, 4.5, sr),   # "thump" de ressonancia do cone
        Biquad.peaking(420.0, 1.6, -3.5, sr),  # carve pra tirar o "mud" caracteristico
        Biquad.peaking(2800.0, 1.1, 5.0, sr),  # presence peak (tipico mic dinamico proximo)
        Biquad.lowpass(5500.0, 0.707, sr),     # rolloff de agudos do cone
    ]


def generate_samples():
    n = int(SAMPLE_RATE * DURATION_S)
    rng = random.Random(42)  # seed fixa -- reprodutivel, nao e criptografia

    impulse = [0.0] * n
    impulse[0] = 1.0

    # Micro-reflexoes: mic + caixa + ar da sala geram pequenas reflexoes nos primeiros ms,
    # nao um impulso unico perfeito.
    for offset_ms, amp in ((0.3, 0.35), (0.7, -0.22), (1.4, 0.15), (2.6, -0.10)):
        idx = int(SAMPLE_RATE * offset_ms / 1000)
        if idx < n:
            impulse[idx] += amp

    chain = build_cabinet_filter_chain(SAMPLE_RATE)
    filtered = []
    for x in impulse:
        for biquad in chain:
            x = biquad.process(x)
        filtered.append(x)

    # Envelope em dois estagios: corpo inicial da caixa decai rapido (~15ms), depois uma
    # cauda mais lenta e sutil de ambiencia (~130ms) -- em vez de um unico decaimento
    # exponencial "limpo demais" pra soar sintetico.
    samples = []
    for i, x in enumerate(filtered):
        t = i / SAMPLE_RATE
        fast = math.exp(-t / 0.012)
        slow = math.exp(-t / 0.06) * 0.12
        envelope = fast + slow
        samples.append(x * envelope + rng.uniform(-1, 1) * 0.0008 * envelope)

    peak = max(abs(s) for s in samples) or 1.0
    return [s / peak * 0.9 for s in samples]


def write_wav(path, samples):
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        frames = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples)
        f.writeframes(frames)


if __name__ == "__main__":
    write_wav(OUT_PATH, generate_samples())
    print(f"IR placeholder (cabinet-shaped) escrito em {OUT_PATH}")
