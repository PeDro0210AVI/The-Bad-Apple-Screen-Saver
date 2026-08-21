# The Bad Apple Screensaver

Proyecto 1 de Computación Paralela y Distribuida (UVG, semestre 2 2026):
un screensaver de N partículas que se mueven y rebotan en los bordes de la
ventana, dibujado con **OpenGL + GLUT**, cuyo cálculo de física se paraleliza
con **OpenMP**. Ver `docs/herramienta_grafica.md` para la justificación de la
herramienta gráfica elegida.

## Prueba de concepto

`src/main.c` es la prueba de concepto: crea la ventana con GLUT y dibuja
partículas (puntos blancos) que se mueven y rebotan en los bordes.

### Dependencias

- Compilador C con soporte OpenMP (`gcc`/`clang`).
- OpenGL + GLUT/freeglut:
  - Linux/WSL: `sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev libomp-dev`
  - Windows (MSYS2): `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-freeglut mingw-w64-x86_64-mesa`
  - macOS: incluido con Xcode Command Line Tools (`OpenGL.framework`, `GLUT.framework`)

### Compilar y correr

```bash
make build
./bin/screensaver
```

```bash
make clean   # limpia bin/ y build/
```

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
