/* ------------------------------------------------------------------------
 * Fisica del screensaver. Por cada particula i y por cada frame:
 *   1. Repulsion con las demas particulas que la tocan (colisiones, O(N^2)).
 *   2. Atraccion tipo resorte hacia un punto de la silueta Bad Apple, o, si
 *      no hay silueta, una fuerza de giro alrededor del centro calculada con
 *      atan2/sin/cos.
 *   3. Integracion de Euler semi-implicita + amortiguamiento.
 *   4. Rebote con los bordes del canvas (se invierte la velocidad).
 *
 * El calculo de una particula esta en funciones "static inline" de este
 * archivo, asi las tres versiones (secuencial, paralela v1 y paralela v2)
 * hacen exactamente las mismas cuentas y solo cambia como se reparte el
 * trabajo entre hilos.
 * -------------------------------------------------------------------------*/

#ifndef FISICA_H
#define FISICA_H

#include <math.h>

#include "sistema.h"

#define DT_SIMULACION (1.0f / 60.0f) /* paso fijo de tiempo (s)         */
#define K_REPULSION 900.0f           /* rigidez del choque              */
#define K_ATRACCION 80.0f            /* resorte hacia la silueta        */
#define K_GIRO 60.0f                 /* fuerza tangencial sin silueta   */
#define K_ONDA 40.0f                 /* oscilacion vertical (seno)      */
#define AMORTIGUAMIENTO 0.985f       /* friccion por frame              */
#define AMORTIGUAMIENTO_SILUETA 0.90f /* friccion mayor para seguir el video */
#define RESTITUCION 0.9f             /* energia que se conserva al rebotar */
#define VELOCIDAD_MAXIMA 1200.0f     /* px/s                            */

/* Paso de fisica completo para cada version. t = tiempo simulado (s). */
void fisica_paso_secuencial(Sistema *s, double t);
void fisica_paso_paralelo_v1(Sistema *s, double t, int hilos);
void fisica_paso_paralelo_v2(Sistema *s, double t, int hilos);

/* ---------------------------------------------------------------------
 * Fuerza de repulsion sobre i: recorre las N particulas (parte O(N^2),
 * la mas costosa y la que justifica paralelizar).
 * ---------------------------------------------------------------------*/
static inline int fuerza_repulsion(const Sistema *s, int i, float *fx,
                                   float *fy) {
  const float xi = s->x[i], yi = s->y[i];
  const float diametro = 2.0f * s->radio;
  const float diametro2 = diametro * diametro;
  float ax = 0.0f, ay = 0.0f;
  int contactos = 0;
  for (int j = 0; j < s->n; j++) {
    float dx = xi - s->x[j];
    float dy = yi - s->y[j];
    float d2 = dx * dx + dy * dy;
    if (d2 < diametro2 && d2 > 1e-6f) {
      float d = sqrtf(d2);
      float f = K_REPULSION * (diametro - d) / d; /* empuje proporcional */
      ax += f * dx;
      ay += f * dy;
      contactos += (j > i);
    }
  }
  *fx += ax;
  *fy += ay;
  return contactos;
}

/* ---------------------------------------------------------------------
 * Fuerzas externas + integracion + rebote. Solo escribe en la posicion i
 * de los buffers "_sig", por eso es seguro llamarla desde varios hilos.
 * ---------------------------------------------------------------------*/
static inline float integrar_particula(Sistema *s, int i, float fx, float fy,
                                       double t) {
  const float xi = s->x[i], yi = s->y[i];

  if (s->num_pix > 0) {
    /* Cada particula persigue una celda de la silueta. El reparto
     * i -> celda es proporcional, asi todas las celdas reciben particulas */
    int celda = (int)(((long long)i * s->num_pix) / s->n);
    fx += K_ATRACCION * (s->pix_x[celda] - xi);
    fy += K_ATRACCION * (s->pix_y[celda] - yi);
  } else {
    /* Sin silueta: giro alrededor del centro (trigonometria) */
    float cx = s->ancho * 0.5f, cy = s->alto * 0.5f;
    float angulo = atan2f(yi - cy, xi - cx);
    fx += -sinf(angulo) * K_GIRO;
    fy += cosf(angulo) * K_GIRO;
    fy += K_ONDA * sinf((float)t * 2.0f + xi * 0.01f);
  }

  /* Euler semi-implicito: primero velocidad, luego posicion. Con silueta
   * se frena mas para que las particulas no "orbiten" su destino. */
  float friccion = s->num_pix > 0 ? AMORTIGUAMIENTO_SILUETA : AMORTIGUAMIENTO;
  float vx = (s->vx[i] + fx * DT_SIMULACION) * friccion;
  float vy = (s->vy[i] + fy * DT_SIMULACION) * friccion;
  float rapidez = sqrtf(vx * vx + vy * vy);
  if (rapidez > VELOCIDAD_MAXIMA) {
    vx *= VELOCIDAD_MAXIMA / rapidez;
    vy *= VELOCIDAD_MAXIMA / rapidez;
  }
  float x = xi + vx * DT_SIMULACION;
  float y = yi + vy * DT_SIMULACION;

  /* Rebote con los bordes: se corrige la posicion y se invierte la
   * componente de la velocidad perdiendo un poco de energia */
  const float r = s->radio;
  if (x < r) {
    x = r;
    vx = fabsf(vx) * RESTITUCION;
  } else if (x > s->ancho - r) {
    x = s->ancho - r;
    vx = -fabsf(vx) * RESTITUCION;
  }
  if (y < r) {
    y = r;
    vy = fabsf(vy) * RESTITUCION;
  } else if (y > s->alto - r) {
    y = s->alto - r;
    vy = -fabsf(vy) * RESTITUCION;
  }

  s->x_sig[i] = x;
  s->y_sig[i] = y;
  s->vx_sig[i] = vx;
  s->vy_sig[i] = vy;
  return 0.5f * (vx * vx + vy * vy);
}

/* Copia la particula i a los buffers de OpenGL. El brillo depende de la
 * rapidez: las particulas rapidas se ven mas brillantes. */
static inline void llenar_buffer_render(Sistema *s, int i) {
  float vx = s->vx_sig[i], vy = s->vy_sig[i];
  float rapidez = sqrtf(vx * vx + vy * vy) / VELOCIDAD_MAXIMA;
  float brillo = 0.55f + 0.45f * (rapidez > 1.0f ? 1.0f : rapidez);
  s->vertices[2 * i] = s->x_sig[i];
  s->vertices[2 * i + 1] = s->y_sig[i];
  s->colores[3 * i] = s->color_base[3 * i] * brillo;
  s->colores[3 * i + 1] = s->color_base[3 * i + 1] * brillo;
  s->colores[3 * i + 2] = s->color_base[3 * i + 2] * brillo;
}

#endif /* FISICA_H */
