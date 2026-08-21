/* ------------------------------------------------------------------------
 * VERSIONES PARALELAS con OpenMP (metodo PCAM):
 *
 *  Particion:    una tarea por particula i (calcular su fuerza y su nuevo
 *                estado). Las tareas son independientes porque leen el
 *                estado actual y escriben solo su posicion i del estado
 *                siguiente (doble buffer).
 *  Comunicacion: todas las tareas leen x/y de todas las particulas
 *                (memoria compartida, solo lectura) y aportan a dos sumas
 *                globales: colisiones y energia.
 *  Aglomeracion: las N tareas se agrupan en bloques contiguos de
 *                particulas, uno por hilo (schedule static).
 *  Mapeo:        un bloque por hilo; OpenMP asigna los hilos a los nucleos.
 *
 * v1: primera version, directa: "parallel for" sobre el ciclo de fuerzas y
 *     otro "parallel for" para el buffer de dibujo. Las sumas globales se
 *     protegen con "atomic".
 * v2: mejora iterativa: una sola region paralela (se crea el equipo de
 *     hilos una vez por frame), "single" para la parte secuencial de la
 *     silueta, "reduction" en lugar de atomic y el ciclo de dibujo
 *     fusionado con el de fuerzas (menos barreras y mejor uso de cache).
 * -------------------------------------------------------------------------*/
#include "fisica.h"

#ifdef _OPENMP
#include <omp.h>
#endif

void fisica_paso_paralelo_v1(Sistema *s, double t, int hilos) {
  sistema_actualizar_silueta(s, t); /* parte secuencial (Amdahl) */

  long colisiones = 0;
  double energia = 0.0;

  /* 1) Fuerzas: cada hilo recibe un bloque de particulas */
#pragma omp parallel for num_threads(hilos) schedule(static)
  for (int i = 0; i < s->n; i++) {
    float fx = 0.0f, fy = 0.0f;
    int c = fuerza_repulsion(s, i, &fx, &fy);
    float e = integrar_particula(s, i, fx, fy, t);
    /* Seccion protegida: varios hilos suman a la misma variable */
#pragma omp atomic
    colisiones += c;
#pragma omp atomic
    energia += e;
  }
  /* Barrera implicita al final del parallel for: todas las particulas ya
   * tienen su estado siguiente antes de llenar el buffer de dibujo. */

  /* 2) Buffer de dibujo */
#pragma omp parallel for num_threads(hilos) schedule(static)
  for (int i = 0; i < s->n; i++) {
    llenar_buffer_render(s, i);
  }

  s->colisiones = colisiones;
  s->energia = energia;
  sistema_intercambiar_buffers(s);
}

void fisica_paso_paralelo_v2(Sistema *s, double t, int hilos) {
  long colisiones = 0;
  double energia = 0.0;

#pragma omp parallel num_threads(hilos) reduction(+ : colisiones, energia)
  {
    /* Un solo hilo actualiza la lista de celdas de la silueta. La barrera
     * implicita al final de "single" garantiza que nadie lea pix_x/pix_y
     * mientras se estan escribiendo. */
#pragma omp single
    sistema_actualizar_silueta(s, t);

    /* Cada hilo acumula en su copia privada de colisiones/energia y
     * OpenMP las suma al final de la region (reduction, sin atomic). Como
     * el buffer de dibujo de i solo depende del nuevo estado de i, se
     * llena en la misma iteracion (fusion de ciclos). */
#pragma omp for schedule(static)
    for (int i = 0; i < s->n; i++) {
      float fx = 0.0f, fy = 0.0f;
      colisiones += fuerza_repulsion(s, i, &fx, &fy);
      energia += integrar_particula(s, i, fx, fy, t);
      llenar_buffer_render(s, i);
    }
    /* Barrera implicita del "for": al salir de la region todos los hilos
     * terminaron y ya se puede intercambiar el doble buffer. */
  }

  s->colisiones = colisiones;
  s->energia = energia;
  sistema_intercambiar_buffers(s);
}
