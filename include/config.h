/* ------------------------------------------------------------------------
 * Configuracion del screensaver leida desde la linea de comandos.
 * Ningun valor importante queda "hard-coded": todo se puede cambiar con
 * argumentos (ver config_imprimir_uso).
 * -------------------------------------------------------------------------*/

#ifndef CONFIG_H
#define CONFIG_H

/* Version del calculo de fisica que se ejecuta */
typedef enum {
  MODO_SECUENCIAL = 0, /* un solo hilo, sin OpenMP                     */
  MODO_PARALELO_V1 = 1, /* parallel for simple + atomic                */
  MODO_PARALELO_V2 = 2  /* region paralela unica, reduction, barreras  */
} ModoEjecucion;

typedef struct {
  int num_particulas;     /* N: cantidad de elementos a renderizar      */
  ModoEjecucion modo;     /* seq, par1 o par2                           */
  int num_hilos;          /* hilos OpenMP (solo modos paralelos)        */
  int ancho;              /* ancho del canvas en pixeles (>= 640)       */
  int alto;               /* alto del canvas en pixeles (>= 480)        */
  float radio;            /* radio de cada particula en pixeles         */
  unsigned int semilla;   /* semilla del generador pseudoaleatorio      */
  const char *ruta_bap;   /* archivo .bap con la silueta (opcional)     */
  int frames_benchmark;   /* > 0: corre esa cantidad de frames y sale   */
} Config;

/* Llena cfg a partir de argc/argv. Retorna 0 si todo es valido, 1 si se
 * pidio --ayuda y -1 si hubo un error (ya impreso en stderr). */
int config_leer_argumentos(int argc, char **argv, Config *cfg);

/* Imprime la forma de uso del programa. */
void config_imprimir_uso(const char *programa);

/* Nombre legible del modo ("secuencial", "paralelo_v1", ...). */
const char *config_nombre_modo(ModoEjecucion modo);

#endif /* CONFIG_H */
