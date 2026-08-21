/* ------------------------------------------------------------------------
 * Estado de la simulacion: N particulas guardadas como "estructura de
 * arreglos" (un arreglo por propiedad) para que cada hilo recorra memoria
 * contigua. Se usa doble buffer: las funciones de fisica LEEN de x/y/vx/vy
 * y ESCRIBEN en x_sig/y_sig/vx_sig/vy_sig, por lo que ningun hilo escribe
 * en algo que otro hilo este leyendo (no hay condiciones de carrera).
 * -------------------------------------------------------------------------*/

#ifndef SISTEMA_H
#define SISTEMA_H

#include <stdint.h>

#include "config.h"
#include "frames.h"

typedef struct {
  int n;        /* cantidad de particulas                         */
  float ancho;  /* tamano del canvas en pixeles                   */
  float alto;
  float radio;  /* radio de cada particula                        */

  /* Estado actual (solo lectura durante un paso de fisica) */
  float *x, *y, *vx, *vy;
  /* Estado siguiente (cada particula i solo escribe su posicion i) */
  float *x_sig, *y_sig, *vx_sig, *vy_sig;

  float *color_base; /* 3*n: RGB pseudoaleatorio de cada particula      */

  /* Buffers que se le pasan a OpenGL para dibujar todo en una llamada */
  float *vertices; /* 2*n: (x, y) */
  float *colores;  /* 3*n: (r, g, b) */

  /* Silueta Bad Apple (opcional). Si frames == NULL las particulas solo
   * giran alrededor del centro y rebotan. */
  Frames *frames;
  int64_t frame_silueta;  /* frame del video cargado en pix_x/pix_y   */
  float *pix_x, *pix_y;   /* centros (en px) de las celdas encendidas */
  int num_pix;            /* cuantas celdas encendidas hay            */
  unsigned char *fondo;   /* cols*rows: silueta tenue que se dibuja
                             detras de las particulas (fila 0 abajo)  */

  /* Estadisticas del ultimo paso */
  long colisiones;     /* pares de particulas en contacto             */
  double energia;      /* energia cinetica total (para depuracion)    */
} Sistema;

/* Reserva memoria e inicializa posiciones, velocidades y colores de forma
 * pseudoaleatoria a partir de cfg->semilla. Retorna NULL si falla. */
Sistema *sistema_crear(const Config *cfg);

/* Libera toda la memoria del sistema (acepta NULL). */
void sistema_destruir(Sistema *s);

/* Si hay silueta cargada, actualiza la lista de celdas encendidas que
 * corresponde al tiempo t (segundos). Es O(cols*rows), una sola vez por
 * frame del video. */
void sistema_actualizar_silueta(Sistema *s, double t);

/* Intercambia el estado actual con el siguiente al final de cada paso. */
void sistema_intercambiar_buffers(Sistema *s);

#endif /* SISTEMA_H */
