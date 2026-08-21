/* ------------------------------------------------------------------------
 * VERSION SECUENCIAL: un solo hilo recorre las N particulas. Es la linea
 * base contra la que se mide el speedup de las versiones paralelas.
 * -------------------------------------------------------------------------*/
#include "fisica.h"

void fisica_paso_secuencial(Sistema *s, double t) {
  sistema_actualizar_silueta(s, t);

  long colisiones = 0;
  double energia = 0.0;

  /* 1) Fuerzas, integracion y rebotes */
  for (int i = 0; i < s->n; i++) {
    float fx = 0.0f, fy = 0.0f;
    colisiones += fuerza_repulsion(s, i, &fx, &fy);
    energia += integrar_particula(s, i, fx, fy, t);
  }

  /* 2) Preparar los buffers que dibuja OpenGL */
  for (int i = 0; i < s->n; i++) {
    llenar_buffer_render(s, i);
  }

  s->colisiones = colisiones;
  s->energia = energia;
  sistema_intercambiar_buffers(s);
}
