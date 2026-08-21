# Investigación de herramienta gráfica: OpenGL + GLUT

## 1. Descripción breve

**OpenGL** (Open Graphics Library) es una API multiplataforma, de bajo nivel, para
renderizado de gráficos 2D y 3D. No provee manejo de ventanas ni de eventos por sí
misma: para eso se usa una librería complementaria. En este proyecto usamos
**GLUT** (OpenGL Utility Toolkit) — concretamente su implementación libre
**freeglut** en Linux/Windows y el framework `GLUT.framework` en macOS — que se
encarga de:

- Crear la ventana y el contexto gráfico (double buffering, RGB).
- Manejar el bucle principal de eventos (`glutMainLoop`).
- Disparar callbacks de dibujado (`glutDisplayFunc`) y de temporizador
  (`glutTimerFunc`), que usamos para animar el screensaver a ~60 FPS.

OpenGL trabaja con primitivas inmediatas (`glBegin`/`glVertex2f`/`glEnd`) que
permiten dibujar miles de elementos (puntos, líneas, triángulos) por frame de
forma directa, sin necesidad de un motor gráfico.

## 2. Justificación

- **Rendimiento con muchos elementos**: el screensaver debe renderizar N
  elementos (partículas) por frame y mantener FPS altos. OpenGL delega el
  dibujado a la GPU, lo cual escala mucho mejor que dibujar N formas con una
  librería basada en render por software o por superficie 2D.
- **Se presta naturalmente al paralelismo con OpenMP**: la parte costosa del
  programa no es el dibujado (eso lo hace la GPU), sino el **cálculo** de la
  física de cada elemento (posición, velocidad, rebotes, colisiones) en cada
  frame. Ese cálculo es un `for` sobre un arreglo de partículas independientes
  entre sí — el caso ideal para `#pragma omp parallel for`, que es justamente
  el requisito central del curso.
- **Separación clara cómputo/dibujado**: la arquitectura callback de GLUT
  (`actualizarParticulas()` antes de `dibujar()`) hace evidente dónde va la
  sección paralela y dónde la secuencial, lo cual ayuda directamente con el
  Anexo 1 (diagrama de flujo, secciones paralelas) pedido en el informe.
- **Ligero y sin dependencias pesadas**: no requiere un motor de juego ni
  librerías adicionales grandes; solo la GPU del sistema, GLUT/freeglut y el
  compilador con soporte OpenMP.
- **Alternativa considerada — SDL2**: SDL2 es más simple para dibujar formas
  2D "de alto nivel" (círculos, rectángulos) y maneja mejor el input, pero para
  miles de partículas por frame requiere iterar y hacer llamadas de dibujo por
  elemento en CPU (o usar el renderer acelerado igualmente apoyado en GPU), por
  lo que la ventaja de rendimiento frente a OpenGL puro es menor. Como el
  equipo ya tenía experiencia y una base de código funcionando en OpenGL/GLUT,
  se optó por continuar con esta opción.

## 3. Requisitos básicos de instalación y uso con C/C++

El programa se compila igual en cualquier sistema (mismo código fuente,
`main.c`), solo cambian los paquetes del sistema operativo:

### Windows (MSYS2 / MinGW-w64)

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-freeglut mingw-w64-x86_64-mesa
```

Enlazar con: `-lfreeglut -lopengl32 -lglu32 -lgomp` (para OpenMP).

### Windows (vía WSL2 / Ubuntu) — usado para probar este PoC

```bash
sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev libomp-dev
```

WSL2 con **WSLg** permite mostrar la ventana directamente en el escritorio de
Windows sin configuración extra de servidor X.

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev libomp-dev
```

### macOS

Xcode Command Line Tools trae OpenGL y GLUT como frameworks del sistema
(`OpenGL.framework`, `GLUT.framework`), no requieren instalación aparte. Para
OpenMP con `clang` se necesita `libomp` (por ejemplo vía `brew install libomp`
o, como en este repo, vía el flake de Nix que ya incluye
`pkgs.llvmPackages.openmp`).

### Flags de compilación (GCC/Clang)

```bash
gcc -fopenmp -Wall -c src/main.c -o main.o
gcc main.o -o screensaver -lfreeglut -lopengl32 -lglu32 -lgomp   # Windows/MinGW
gcc main.o -o screensaver -lGL -lGLU -lglut -lgomp -lm          # Linux
clang main.o -o screensaver -framework OpenGL -framework GLUT -lomp  # macOS
```

El `Makefile` del repositorio ya detecta el sistema operativo (`uname -s`) y
selecciona automáticamente las librerías correctas.

## 4. Ejemplo de referencia

El propio enunciado del proyecto incluye una captura de un screensaver de N
círculos rebotando hecho en C++ con OpenMP, usando gráficos (ver Imagen 1 del
enunciado). Como referencia adicional de una aplicación gráfica hecha con
OpenGL + GLUT: los ejemplos oficiales de freeglut
(`freeglut/progs/demos` en <https://github.com/FreeGLUTProject/freeglut>),
en particular `Fractals` y `Ico`, muestran el mismo patrón de
`glutDisplayFunc` + `glutTimerFunc` que usamos aquí para animar la escena
cuadro a cuadro.
