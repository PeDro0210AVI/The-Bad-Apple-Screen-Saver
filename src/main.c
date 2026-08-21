// GLUT/OpenGL viven en paquetes distintos segun el sistema operativo
#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/freeglut.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "frames.h"

#define DEFAULT_BAP_PATH "misc/bad_apple.bap"
#define ZOOM 12

static Frames *g_frames = NULL;
static unsigned char *g_framebuffer =
    NULL; /* cols * rows, 1 byte/pixel (luminancia) */
static uint32_t g_frame_idx = 0;

void construirFramebuffer() {
  uint32_t cols = g_frames->cols;
  uint32_t rows = g_frames->rows;

  for (uint32_t y = 0; y < rows; y++) {
    for (uint32_t x = 0; x < cols; x++) {
      int on = frames_get_pixel(g_frames, g_frame_idx, x, y);
      g_framebuffer[y * cols + x] = on ? 255 : 0;
    }
  }
}

// ---------------------------------------------------------
// DIBUJADO: OpenGL vuelca el framebuffer a la ventana
// ---------------------------------------------------------
void dibujar() {
  glClear(GL_COLOR_BUFFER_BIT);

  // El framebuffer se guarda con la fila 0 arriba; se ubica el raster
  // position en la esquina superior izquierda y se usa zoom Y negativo
  // para que las filas se dibujen hacia abajo en el orden correcto.
  glRasterPos2f(-1.0f, 1.0f);
  glPixelZoom((float)ZOOM, -(float)ZOOM);
  glDrawPixels(g_frames->cols, g_frames->rows, GL_LUMINANCE, GL_UNSIGNED_BYTE,
               g_framebuffer);

  glutSwapBuffers();
}

// ---------------------------------------------------------
// Temporizador de animacion
// ---------------------------------------------------------
void temporizador(int valor) {
  g_frame_idx = (g_frame_idx + 1) % g_frames->frame_count;
  construirFramebuffer();

  glutPostRedisplay();

  int delay_ms = g_frames->fps > 0.0f ? (int)(1000.0f / g_frames->fps) : 33;
  glutTimerFunc(delay_ms, temporizador, 0);
}

// ---------------------------------------------------------
// Configuracion inicial de OpenGL
// ---------------------------------------------------------
void configurarOpenGL() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  // El ancho de fila del framebuffer no siempre es multiplo de 4
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

// ---------------------------------------------------------
// Programa principal
// ---------------------------------------------------------
int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : DEFAULT_BAP_PATH;

  g_frames = frames_load(path);
  if (!g_frames) {
    fprintf(stderr, "No se pudo cargar '%s'. Uso: %s [archivo.bap]\n", path,
            argv[0]);
    return 1;
  }

  g_framebuffer = malloc((size_t)g_frames->cols * g_frames->rows);
  if (!g_framebuffer) {
    fprintf(stderr, "Sin memoria para el framebuffer\n");
    frames_free(g_frames);
    return 1;
  }
  construirFramebuffer();

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  glutInitWindowSize(g_frames->cols * ZOOM, g_frames->rows * ZOOM);
  glutCreateWindow("Bad Apple Screensaver");
  configurarOpenGL();

  glutDisplayFunc(dibujar);
  int delay_ms = g_frames->fps > 0.0f ? (int)(1000.0f / g_frames->fps) : 33;
  glutTimerFunc(delay_ms, temporizador, 0);

  glutMainLoop();

  free(g_framebuffer);
  frames_free(g_frames);
  return 0;
}
