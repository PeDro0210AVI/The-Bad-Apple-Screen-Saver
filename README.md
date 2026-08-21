# The Bad Apple Screensaver

Proyecto 1 de Computación Paralela y Distribuida (UVG, semestre 2 2026):
un screensaver de **N partículas de colores** que se mueven, chocan entre sí
y rebotan en los bordes de la ventana. Si se le da el archivo `.bap` del video
*Bad Apple*, las partículas son atraídas hacia la silueta de cada cuadro y
"dibujan" el video. Se dibuja con **OpenGL + GLUT** y la física se calcula en
versión **secuencial** o **paralela con OpenMP**.

Ver `docs/herramienta_grafica.md` para la justificación de la herramienta gráfica.

## Estructura

| Archivo | Contenido |
|---|---|
| `src/main.c` | Ventana GLUT, ciclo principal, dibujo, FPS y modo benchmark |
| `src/config.c` | Lectura y validación de argumentos (programación defensiva) |
| `src/sistema.c` | Memoria de las partículas, colores pseudoaleatorios y silueta |
| `include/fisica.h` | Física de una partícula: choques, atracción/giro, rebotes |
| `src/fisica_secuencial.c` | Versión secuencial |
| `src/fisica_paralela.c` | Versiones paralelas v1 y v2 (OpenMP) |
| `src/frames.c` | Lector del formato `.bap` |
| `scripts/extract_frames.py` | Convierte el video a `.bap` (Python + OpenCV) |
| `scripts/benchmark.sh` | Bitácora de pruebas (10 corridas por configuración) |
| `scripts/analizar_benchmark.py` | Promedios, speedup y eficiencia |

## Dependencias

- Compilador C con soporte OpenMP (`gcc`/`clang`).
- OpenGL + GLUT/freeglut:
  - Linux/WSL: `sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev libomp-dev`
  - Windows (MSYS2): `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-freeglut mingw-w64-x86_64-mesa`
  - macOS: OpenGL/GLUT vienen con Xcode Command Line Tools; OpenMP con `libomp` (brew o el flake de Nix).

## Compilar y correr

```bash
make build
./bin/screensaver 3000                       # N = 3000, version paralela v2
./bin/screensaver 3000 --modo seq            # version secuencial
./bin/screensaver 3000 --modo par1 --hilos 4 # paralela v1 con 4 hilos
make run N=5000 MODO=par2
make clean
```

### Parámetros

| Parámetro | Descripción | Default |
|---|---|---|
| `N` (obligatorio) | Cantidad de partículas (1–200000) | — |
| `--modo seq\|par1\|par2` | Versión de la física | `par2` |
| `--hilos T` | Hilos OpenMP | núcleos de la máquina |
| `--ancho W` / `--alto H` | Canvas (mínimo 640x480) | 800x600 |
| `--radio R` | Radio de cada partícula (px) | 3 |
| `--semilla S` | Semilla pseudoaleatoria | 42 |
| `--bap RUTA` | Silueta Bad Apple (opcional) | `misc/bad_apple.bap` |
| `--benchmark F` | Corre F frames, imprime una línea CSV y sale | — |

Los valores inválidos (texto en vez de número, N fuera de rango, canvas menor a
640x480, opciones desconocidas) se reportan con un mensaje y la forma de uso.
`ESC` o `q` cierran la ventana. Los FPS se muestran en pantalla, en el título
de la ventana y en consola cada segundo.

### Silueta Bad Apple (opcional)

El video y el `.bap` no se suben al repo por derechos de autor. Para generarlo:

```bash
pip install -r scripts/requirements.txt
python scripts/extract_frames.py --video misc/bad_apple.mp4 --out misc/bad_apple.bap --cols 96 --fps 30
```

Sin el archivo, el screensaver funciona igual con giro alrededor del centro y rebotes.

## Mediciones (speedup y eficiencia)

```bash
make build
./scripts/benchmark.sh                  # o: make bench
NS="2000 6000" HILOS="4 8" ./scripts/benchmark.sh
```

Genera `results/benchmark.csv` (una línea por corrida) y
`results/benchmark_resumen.csv` con el promedio de 10 corridas,
`speedup = T_secuencial / T_paralelo` y `eficiencia = speedup / hilos`.

## Flake de desarrollo (Nix)

Este repo usa como base un flake de Nix para el entorno de desarrollo en C.

### Requirements

- Nix flake enabled on any nix system or system with nix.

- direnv (needed if you want to setup auto startup for flake on entering by the shell).

## How to setup

### Vanilla

- Init flake template.

```bash
nix flake init -t github:PeDro0210/c-clang-base-dev-flake#default
```

- You are all set!

### With direnv

- Install direnv

- Init flake template.

```bash
nix flake init -t github:PeDro0210/c-clang-base-dev-flake#default
```

- Allow direnv.

```bash
direnv allow
```
