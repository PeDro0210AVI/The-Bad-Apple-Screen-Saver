/* ------------------------------------------------------------------------
 * Universidad del Valle de Guatemala
 * Curso: CC3069 – Computación Paralela y Distribuida
 * Sección: 30
 * Fecha: 08/12/2026
 * Descripción: Prueba de concepto del screensaver. Crea una ventana con
 *   OpenGL/GLUT y dibuja N partículas que se mueven y rebotan en los bordes.
 * -------------------------------------------------------------------------*/

// GLUT/OpenGL viven en paquetes distintos segun el sistema operativo
#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/freeglut.h>
#include <GL/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#define NUM_PARTICULAS 100000

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
} Particula;

Particula particulas[NUM_PARTICULAS];

// ---------------------------------------------------------
// Inicializar particulas
// ---------------------------------------------------------
void inicializarParticulas() {
  for (int i = 0; i < NUM_PARTICULAS; i++) {
    particulas[i].x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    particulas[i].y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    particulas[i].vx = ((float)rand() / RAND_MAX) * 0.01f - 0.005f;
    particulas[i].vy = ((float)rand() / RAND_MAX) * 0.01f - 0.005f;
  }
}
// ---------------------------------------------------------
// CALCULO: actualizar posicion de las particulas
// ---------------------------------------------------------
void actualizarParticulas() {
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < NUM_PARTICULAS; i++) {

    particulas[i].x += particulas[i].vx;
    particulas[i].y += particulas[i].vy;

    // Si la particula llega al borde horizontal,
    // cambia la direccion de movimiento
    if (particulas[i].x >= 1.0f || particulas[i].x <= -1.0f) {
      particulas[i].vx = -particulas[i].vx;
    }
    // Si la particula llega al borde vertical,
    // cambia la direccion de movimiento
    if (particulas[i].y >= 1.0f || particulas[i].y <= -1.0f) {
      particulas[i].vy = -particulas[i].vy;
    }
  }
}
// ---------------------------------------------------------
// DIBUJADO: OpenGL dibuja las particulas
// ---------------------------------------------------------
void dibujar() {
  glClear(GL_COLOR_BUFFER_BIT);

  // Primero se actualizan las posiciones
  actualizarParticulas();

  // Dibujar todas las particulas
  glPointSize(3.0f);
  glBegin(GL_POINTS);

  for (int i = 0; i < NUM_PARTICULAS; i++) {
    glColor3f(1.0f, 1.0f, 1.0f);

    glVertex2f(particulas[i].x, particulas[i].y);
  }
  glEnd();
  glutSwapBuffers();
}

// ---------------------------------------------------------
// Temporizador de animacion
// ---------------------------------------------------------
void temporizador(int valor) {

  glutPostRedisplay();
  glutTimerFunc(16, temporizador, 0);
}
// ---------------------------------------------------------
// Configuracion inicial de OpenGL
// ---------------------------------------------------------

void configurarOpenGL() { glClearColor(0.0f, 0.0f, 0.0f, 1.0f); }

// ---------------------------------------------------------
// Programa principal
// ---------------------------------------------------------
int main(int argc, char **argv) {

  inicializarParticulas();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  glutInitWindowSize(800, 600);
  glutCreateWindow("Simulacion de Particulas");
  configurarOpenGL();

  glutDisplayFunc(dibujar);
  glutTimerFunc(16, temporizador, 0);

  glutMainLoop();

  return 0;
}
