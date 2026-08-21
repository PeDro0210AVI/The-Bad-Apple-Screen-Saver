#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Bitacora de pruebas: corre cada configuracion REPS veces (minimo 10) y
# guarda una linea CSV por corrida en results/benchmark.csv.
#
# Variables opcionales:
#   NS="1000 2000 4000 8000"   valores de N a probar
#   HILOS="2 4 8"              hilos para las versiones paralelas
#   REPS=10                    repeticiones por configuracion
#   FRAMES=120                 frames por corrida
#   BAP=misc/bad_apple.bap     silueta (si no existe se usa modo rebotes)
#
# Ejemplo: NS="2000 6000" HILOS="4 8" ./scripts/benchmark.sh
# ---------------------------------------------------------------------------
set -euo pipefail

BIN=./bin/screensaver
NS=${NS:-"1000 2000 4000 8000"}
REPS=${REPS:-10}
FRAMES=${FRAMES:-120}
BAP=${BAP:-misc/bad_apple.bap}
OUT=${OUT:-results/benchmark.csv}

# Hilos por defecto: 2, 4 y la cantidad de nucleos de la maquina
NUCLEOS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu)
HILOS=${HILOS:-"$(echo 2 4 "$NUCLEOS" | tr ' ' '\n' | sort -nu | tr '\n' ' ')"}

if [ ! -x "$BIN" ]; then
  echo "No existe $BIN, corre 'make build' primero" >&2
  exit 1
fi
if [ "$REPS" -lt 10 ]; then
  echo "REPS debe ser al menos 10 (requisito de la bitacora)" >&2
  exit 1
fi
[ -f "$BAP" ] || BAP=none

mkdir -p "$(dirname "$OUT")"
echo "repeticion,modo,hilos,n,frames,ms_fisica_prom,ms_frame_prom,fps_prom" > "$OUT"

correr() { # $1=rep, resto = argumentos del screensaver
  local rep=$1; shift
  local linea
  linea=$("$BIN" "$@" --benchmark "$FRAMES" --bap "$BAP" 2>/dev/null)
  echo "$rep,$linea" >> "$OUT"
  echo "  rep $rep: $linea"
}

for n in $NS; do
  echo "== N=$n"
  for rep in $(seq 1 "$REPS"); do
    correr "$rep" "$n" --modo seq
    for h in $HILOS; do
      correr "$rep" "$n" --modo par1 --hilos "$h"
      correr "$rep" "$n" --modo par2 --hilos "$h"
    done
  done
done

echo "Listo: $OUT"
python3 scripts/analizar_benchmark.py "$OUT" || true
