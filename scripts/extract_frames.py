#!/usr/bin/env python3
"""Convierte un video local en un patron binario por frame (silueta on/off)
que el screensaver en C puede cargar para animar N particulas.

No guarda imagenes de los frames: cada frame se reduce a una grilla de
cols x rows bits (1 = silueta / pixel oscuro, 0 = fondo), empacados en un
archivo binario compacto (.bap) pensado solo como dato de entrada para la
simulacion, no como copia del video.

Uso:
    python extract_frames.py --video misc/bad_apple.mp4 --out misc/bad_apple.bap \
        --cols 120 --fps 30
"""

import argparse
import struct
import sys

import cv2
import numpy as np

MAGIC = b"BADA"


def parse_args():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--video", required=True, help="Ruta al archivo de video local")
    p.add_argument("--out", required=True, help="Ruta de salida del binario .bap")
    p.add_argument("--cols", type=int, default=120, help="Columnas de la grilla (ancho)")
    p.add_argument("--rows", type=int, default=0,
                   help="Filas de la grilla (alto). 0 = calcular segun el aspect ratio del video")
    p.add_argument("--threshold", type=int, default=127,
                   help="Umbral 0-255 para decidir silueta vs fondo")
    p.add_argument("--invert", action="store_true",
                   help="Invertir polaridad (usar si la silueta sale en blanco sobre fondo negro)")
    p.add_argument("--fps", type=float, default=0,
                   help="FPS de salida (submuestrea el video). 0 = usar el fps original")
    p.add_argument("--max-frames", type=int, default=0, help="Limite de frames (0 = todos)")
    return p.parse_args()


def main():
    args = parse_args()

    cap = cv2.VideoCapture(args.video)
    if not cap.isOpened():
        sys.exit(f"No se pudo abrir el video: {args.video}")

    src_fps = cap.get(cv2.CAP_PROP_FPS) or 30.0
    src_w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    src_h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    cols = args.cols
    rows = args.rows or max(1, round(cols * src_h / src_w))
    out_fps = args.fps or src_fps
    stride = max(1, round(src_fps / out_fps))

    print(f"Video: {src_w}x{src_h} @ {src_fps:.2f} fps")
    print(f"Grilla de salida: {cols}x{rows} @ {out_fps:.2f} fps (stride={stride})")

    row_bytes = (cols + 7) // 8
    frame_payload_size = row_bytes * rows

    frames_written = 0
    idx = 0
    packed_frames = []

    while True:
        ok, frame = cap.read()
        if not ok:
            break

        if idx % stride == 0:
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            small = cv2.resize(gray, (cols, rows), interpolation=cv2.INTER_AREA)

            on = small < args.threshold
            if args.invert:
                on = ~on

            packed = np.packbits(on, axis=1)
            if packed.shape[1] != row_bytes:
                # packbits ya redondea al byte, pero por seguridad recortamos/rellenamos
                packed = packed[:, :row_bytes]

            packed_frames.append(packed.tobytes())
            frames_written += 1

            if args.max_frames and frames_written >= args.max_frames:
                break

        idx += 1

    cap.release()

    if frames_written == 0:
        sys.exit("No se extrajo ningun frame (revisa la ruta del video).")

    with open(args.out, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<IIIf", cols, rows, frames_written, out_fps))
        for payload in packed_frames:
            f.write(payload)

    total_bytes = 12 + frame_payload_size * frames_written
    print(f"Listo: {frames_written} frames -> {args.out} ({total_bytes / 1024:.1f} KiB)")


if __name__ == "__main__":
    main()
