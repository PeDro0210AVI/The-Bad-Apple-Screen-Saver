#!/usr/bin/env python3
"""Resume results/benchmark.csv: promedio de cada configuracion, speedup y
eficiencia respecto a la version secuencial con el mismo N.

    speedup    = T_secuencial_promedio / T_paralelo_promedio
    eficiencia = speedup / hilos

T es el tiempo promedio de fisica por frame (ms_fisica_prom). Solo usa la
libreria estandar de Python.

Uso: python3 scripts/analizar_benchmark.py [results/benchmark.csv]
"""

import csv
import statistics
import sys
from collections import defaultdict


def main():
    ruta = sys.argv[1] if len(sys.argv) > 1 else "results/benchmark.csv"
    grupos = defaultdict(lambda: {"fisica": [], "frame": [], "fps": []})
    with open(ruta, newline="") as f:
        for fila in csv.DictReader(f):
            clave = (int(fila["n"]), fila["modo"], int(fila["hilos"]))
            grupos[clave]["fisica"].append(float(fila["ms_fisica_prom"]))
            grupos[clave]["frame"].append(float(fila["ms_frame_prom"]))
            grupos[clave]["fps"].append(float(fila["fps_prom"]))

    filas = []
    for (n, modo, hilos), g in sorted(grupos.items()):
        t_seq = statistics.mean(grupos[(n, "secuencial", 1)]["fisica"]) \
            if (n, "secuencial", 1) in grupos else None
        t = statistics.mean(g["fisica"])
        speedup = t_seq / t if t_seq else None
        filas.append({
            "n": n, "modo": modo, "hilos": hilos, "corridas": len(g["fisica"]),
            "ms_fisica_prom": round(t, 3),
            "ms_fisica_desv": round(statistics.stdev(g["fisica"]), 3)
            if len(g["fisica"]) > 1 else 0.0,
            "ms_frame_prom": round(statistics.mean(g["frame"]), 3),
            "fps_prom": round(statistics.mean(g["fps"]), 2),
            "speedup": round(speedup, 3) if speedup else "",
            "eficiencia": round(speedup / hilos, 3) if speedup else "",
        })

    salida = ruta.replace(".csv", "_resumen.csv")
    with open(salida, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(filas[0].keys()))
        w.writeheader()
        w.writerows(filas)

    print(f"{'N':>6} {'modo':<12} {'hilos':>5} {'ms fisica':>10} "
          f"{'FPS':>8} {'speedup':>8} {'efic.':>6}")
    for r in filas:
        print(f"{r['n']:>6} {r['modo']:<12} {r['hilos']:>5} "
              f"{r['ms_fisica_prom']:>10} {r['fps_prom']:>8} "
              f"{str(r['speedup']):>8} {str(r['eficiencia']):>6}")
    print(f"\nResumen guardado en {salida}")


if __name__ == "__main__":
    main()
