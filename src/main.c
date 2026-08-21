/* ------------------------------------------------------------------------
 * The Bad Apple Screensaver - Proyecto 1, Computacion Paralela (UVG 2026)
 *
 * Screensaver de N particulas de colores que rebotan, chocan entre si y
 * (si hay archivo .bap) forman la silueta del video "Bad Apple". Dibuja
 * con OpenGL + GLUT y calcula la fisica en version secuencial o paralela
 * (OpenMP), segun --modo.
 * -------------------------------------------------------------------------*/

// GLUT/OpenGL viven en paquetes distintos segun el sistema operativo
#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/freeglut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#else
#include <time.h>
#endif

#include "config.h"
#include "fisica.h"
#include "sistema.h"

/* Estado global (GLUT usa callbacks sin parametros de usuario) */
static Config g_cfg;
static Sistema *g_sistema = NULL;
static long g_frames_totales = 0;  /* frames dibujados desde el inicio */
static double g_tiempo_simulado = 0.0;

/* Medicion de FPS */
static double g_inicio_segundo = 0.0; /* inicio de la ventana de 1 s   */
static int g_frames_segundo = 0;      /* frames dentro de esa ventana  */
static double g_fps_actual = 0.0;     /* ultimo FPS calculado          */

/* Acumuladores para --benchmark */
static double g_inicio_benchmark = 0.0;
static double g_suma_fisica = 0.0; /* segundos totales en la fisica   */

/* Reloj de pared en segundos */
static double tiempo_actual(void) {
#ifdef _OPENMP
  return omp_get_wtime();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

/* Libera la memoria al salir (registrada con atexit, porque glutMainLoop
 * no retorna en todas las implementaciones de GLUT). */
static void liberar_recursos(void) {
  sistema_destruir(g_sistema);
  g_sistema = NULL;
}

/* Ejecuta un paso de fisica con la version elegida y devuelve cuanto
 * tardo (en segundos). */
static double ejecutar_paso_fisica(void) {
  double t0 = tiempo_actual();
  switch (g_cfg.modo) {
  case MODO_SECUENCIAL:
    fisica_paso_secuencial(g_sistema, g_tiempo_simulado);
    break;
  case MODO_PARALELO_V1:
    fisica_paso_paralelo_v1(g_sistema, g_tiempo_simulado, g_cfg.num_hilos);
    break;
  case MODO_PARALELO_V2:
    fisica_paso_paralelo_v2(g_sistema, g_tiempo_simulado, g_cfg.num_hilos);
    break;
  }
  g_tiempo_simulado += DT_SIMULACION;
  return tiempo_actual() - t0;
}

/* Escribe texto en pantalla en la posicion (x, y) en pixeles */
static void dibujar_texto(float x, float y, const char *texto) {
  glRasterPos2f(x, y);
  for (const char *c = texto; *c; c++) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }
}

// ---------------------------------------------------------
// DIBUJADO: todas las particulas en una sola llamada (vertex arrays)
// ---------------------------------------------------------
static void dibujar(void) {
  glClear(GL_COLOR_BUFFER_BIT);

  /* Silueta Bad Apple en gris tenue, escalada al tamano del canvas */
  if (g_sistema->frames) {
    glRasterPos2f(0.0f, 0.0f);
    glPixelZoom(g_cfg.ancho / (float)g_sistema->frames->cols,
                g_cfg.alto / (float)g_sistema->frames->rows);
    glDrawPixels(g_sistema->frames->cols, g_sistema->frames->rows,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, g_sistema->fondo);
  }

  glVertexPointer(2, GL_FLOAT, 0, g_sistema->vertices);
  glColorPointer(3, GL_FLOAT, 0, g_sistema->colores);
  glDrawArrays(GL_POINTS, 0, g_sistema->n);

  /* Panel con FPS y datos de la corrida */
  char texto[128];
  glColor3f(1.0f, 1.0f, 1.0f);
  snprintf(texto, sizeof(texto), "FPS: %.1f", g_fps_actual);
  dibujar_texto(10.0f, g_cfg.alto - 24.0f, texto);
  snprintf(texto, sizeof(texto), "N=%d  %s  hilos=%d  choques=%ld",
           g_sistema->n, config_nombre_modo(g_cfg.modo), g_cfg.num_hilos,
           g_sistema->colisiones);
  dibujar_texto(10.0f, g_cfg.alto - 46.0f, texto);

  glutSwapBuffers();
}

/* Imprime el resultado del benchmark como linea CSV y termina */
static void terminar_benchmark(void) {
  double total = tiempo_actual() - g_inicio_benchmark;
  int frames = g_cfg.frames_benchmark;
  /* modo,hilos,n,frames,ms_fisica_prom,ms_frame_prom,fps_prom */
  printf("%s,%d,%d,%d,%.4f,%.4f,%.2f\n", config_nombre_modo(g_cfg.modo),
         g_cfg.num_hilos, g_sistema->n, frames,
         1000.0 * g_suma_fisica / frames, 1000.0 * total / frames,
         frames / total);
  fflush(stdout);
  exit(0); /* atexit -> liberar_recursos */
}

// ---------------------------------------------------------
// CICLO PRINCIPAL: GLUT lo llama cuando no hay eventos pendientes,
// asi el programa corre tan rapido como pueda (FPS sin limite fijo).
// ---------------------------------------------------------
static void ciclo(void) {
  double duracion_fisica = ejecutar_paso_fisica();
  glutPostRedisplay();

  g_frames_totales++;
  g_frames_segundo++;

  double ahora = tiempo_actual();
  if (ahora - g_inicio_segundo >= 1.0) {
    g_fps_actual = g_frames_segundo / (ahora - g_inicio_segundo);
    g_frames_segundo = 0;
    g_inicio_segundo = ahora;
    if (g_cfg.frames_benchmark == 0) {
      printf("FPS= %.2f  (fisica: %.2f ms)\n", g_fps_actual,
             duracion_fisica * 1000.0);
      char titulo[96];
      snprintf(titulo, sizeof(titulo), "Bad Apple Screensaver - FPS= %.2f",
               g_fps_actual);
      glutSetWindowTitle(titulo);
    }
  }

  if (g_cfg.frames_benchmark > 0) {
    g_suma_fisica += duracion_fisica;
    if (g_frames_totales >= g_cfg.frames_benchmark) {
      terminar_benchmark();
    }
  }
}

/* ESC o q cierran el programa */
static void teclado(unsigned char tecla, int x, int y) {
  (void)x;
  (void)y;
  if (tecla == 27 || tecla == 'q' || tecla == 'Q') {
    exit(0);
  }
}

/* Mantiene el sistema de coordenadas en pixeles del canvas aunque la
 * ventana cambie de tamano. */
static void redimensionar(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, g_cfg.ancho, 0.0, g_cfg.alto, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

// ---------------------------------------------------------
// Configuracion inicial de OpenGL
// ---------------------------------------------------------
static void configurar_opengl(void) {
  glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
  /* Puntos redondos y suavizados del tamano de la particula */
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(2.0f * g_cfg.radio);
  /* El ancho de fila de la silueta no siempre es multiplo de 4 */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
}

// ---------------------------------------------------------
// Programa principal
// ---------------------------------------------------------
int main(int argc, char **argv) {
  /* Primero se validan los argumentos (programacion defensiva): si hay
   * un error se termina antes de abrir la ventana. */
  /* Salida linea por linea aunque se redirija a un archivo */
  setvbuf(stdout, NULL, _IOLBF, 0);

  int estado = config_leer_argumentos(argc, argv, &g_cfg);
  if (estado != 0) {
    config_imprimir_uso(argv[0]);
    return estado > 0 ? 0 : 1;
  }

  g_sistema = sistema_crear(&g_cfg);
  if (!g_sistema) {
    return 1;
  }
  atexit(liberar_recursos);

  if (g_cfg.frames_benchmark == 0) {
    printf("N=%d modo=%s hilos=%d canvas=%dx%d\n", g_cfg.num_particulas,
           config_nombre_modo(g_cfg.modo), g_cfg.num_hilos, g_cfg.ancho,
           g_cfg.alto);
  }

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(g_cfg.ancho, g_cfg.alto);
  glutCreateWindow("Bad Apple Screensaver");
  configurar_opengl();

  glutDisplayFunc(dibujar);
  glutReshapeFunc(redimensionar);
  glutKeyboardFunc(teclado);
  glutIdleFunc(ciclo);

  g_inicio_segundo = tiempo_actual();
  g_inicio_benchmark = g_inicio_segundo;

  glutMainLoop();
  return 0;
}
