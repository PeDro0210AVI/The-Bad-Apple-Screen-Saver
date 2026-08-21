#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

/* Limites para la programacion defensiva */
#define N_MAXIMO 200000
#define HILOS_MAXIMO 256
#define ANCHO_MINIMO 640
#define ALTO_MINIMO 480
#define LADO_MAXIMO 7680
#define RADIO_MINIMO 1.0f
#define RADIO_MAXIMO 50.0f

/* Convierte texto a entero validando que sea un numero completo y que este
 * dentro de [minimo, maximo]. Retorna 0 si es valido, -1 si no. */
static int leer_entero(const char *nombre, const char *texto, long minimo,
                       long maximo, long *salida) {
  if (texto == NULL) {
    fprintf(stderr, "Error: falta el valor de %s\n", nombre);
    return -1;
  }
  char *fin = NULL;
  errno = 0;
  long valor = strtol(texto, &fin, 10);
  if (errno != 0 || fin == texto || *fin != '\0') {
    fprintf(stderr, "Error: %s debe ser un entero, se recibio '%s'\n", nombre,
            texto);
    return -1;
  }
  if (valor < minimo || valor > maximo) {
    fprintf(stderr, "Error: %s debe estar entre %ld y %ld (se recibio %ld)\n",
            nombre, minimo, maximo, valor);
    return -1;
  }
  *salida = valor;
  return 0;
}

/* Igual que leer_entero pero para numeros con decimales. */
static int leer_flotante(const char *nombre, const char *texto, float minimo,
                         float maximo, float *salida) {
  if (texto == NULL) {
    fprintf(stderr, "Error: falta el valor de %s\n", nombre);
    return -1;
  }
  char *fin = NULL;
  errno = 0;
  float valor = strtof(texto, &fin);
  if (errno != 0 || fin == texto || *fin != '\0') {
    fprintf(stderr, "Error: %s debe ser un numero, se recibio '%s'\n", nombre,
            texto);
    return -1;
  }
  if (valor < minimo || valor > maximo) {
    fprintf(stderr, "Error: %s debe estar entre %.1f y %.1f\n", nombre,
            minimo, maximo);
    return -1;
  }
  *salida = valor;
  return 0;
}

const char *config_nombre_modo(ModoEjecucion modo) {
  switch (modo) {
  case MODO_SECUENCIAL:
    return "secuencial";
  case MODO_PARALELO_V1:
    return "paralelo_v1";
  case MODO_PARALELO_V2:
    return "paralelo_v2";
  }
  return "desconocido";
}

void config_imprimir_uso(const char *programa) {
  fprintf(stderr,
          "Uso: %s N [opciones]\n"
          "\n"
          "  N                    cantidad de particulas a renderizar (1-%d)\n"
          "\n"
          "Opciones:\n"
          "  --modo seq|par1|par2 version de la fisica (default: par2)\n"
          "  --hilos T            hilos OpenMP (default: todos los nucleos)\n"
          "  --ancho W            ancho del canvas, minimo %d (default: 800)\n"
          "  --alto H             alto del canvas, minimo %d (default: 600)\n"
          "  --radio R            radio de cada particula en px (default: 3)\n"
          "  --semilla S          semilla pseudoaleatoria (default: 42)\n"
          "  --bap RUTA           silueta Bad Apple (default: "
          "misc/bad_apple.bap)\n"
          "  --benchmark F        corre F frames, imprime una linea CSV y "
          "sale\n"
          "  --ayuda              muestra esta ayuda\n"
          "\n"
          "Ejemplo: %s 3000 --modo par2 --hilos 8\n",
          programa, N_MAXIMO, ANCHO_MINIMO, ALTO_MINIMO, programa);
}

int config_leer_argumentos(int argc, char **argv, Config *cfg) {
  /* Valores por defecto */
  cfg->num_particulas = 0;
  cfg->modo = MODO_PARALELO_V2;
#ifdef _OPENMP
  cfg->num_hilos = omp_get_num_procs();
#else
  cfg->num_hilos = 1;
#endif
  cfg->ancho = 800;
  cfg->alto = 600;
  cfg->radio = 3.0f;
  cfg->semilla = 42;
  cfg->ruta_bap = "misc/bad_apple.bap";
  cfg->frames_benchmark = 0;

  int n_recibido = 0;
  long valor = 0;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    /* Valor que sigue a la opcion (NULL si no hay) */
    const char *siguiente = (i + 1 < argc) ? argv[i + 1] : NULL;

    if (strcmp(arg, "--ayuda") == 0 || strcmp(arg, "--help") == 0) {
      return 1;
    } else if (strcmp(arg, "--modo") == 0) {
      if (siguiente == NULL) {
        fprintf(stderr, "Error: falta el valor de --modo\n");
        return -1;
      }
      if (strcmp(siguiente, "seq") == 0) {
        cfg->modo = MODO_SECUENCIAL;
      } else if (strcmp(siguiente, "par1") == 0) {
        cfg->modo = MODO_PARALELO_V1;
      } else if (strcmp(siguiente, "par2") == 0) {
        cfg->modo = MODO_PARALELO_V2;
      } else {
        fprintf(stderr, "Error: --modo debe ser seq, par1 o par2\n");
        return -1;
      }
      i++;
    } else if (strcmp(arg, "--hilos") == 0) {
      if (leer_entero("--hilos", siguiente, 1, HILOS_MAXIMO, &valor) != 0)
        return -1;
      cfg->num_hilos = (int)valor;
      i++;
    } else if (strcmp(arg, "--ancho") == 0) {
      if (leer_entero("--ancho", siguiente, ANCHO_MINIMO, LADO_MAXIMO,
                      &valor) != 0)
        return -1;
      cfg->ancho = (int)valor;
      i++;
    } else if (strcmp(arg, "--alto") == 0) {
      if (leer_entero("--alto", siguiente, ALTO_MINIMO, LADO_MAXIMO, &valor) !=
          0)
        return -1;
      cfg->alto = (int)valor;
      i++;
    } else if (strcmp(arg, "--radio") == 0) {
      if (leer_flotante("--radio", siguiente, RADIO_MINIMO, RADIO_MAXIMO,
                        &cfg->radio) != 0)
        return -1;
      i++;
    } else if (strcmp(arg, "--semilla") == 0) {
      if (leer_entero("--semilla", siguiente, 0, INT_MAX, &valor) != 0)
        return -1;
      cfg->semilla = (unsigned int)valor;
      i++;
    } else if (strcmp(arg, "--bap") == 0) {
      if (siguiente == NULL) {
        fprintf(stderr, "Error: falta el valor de --bap\n");
        return -1;
      }
      cfg->ruta_bap = siguiente;
      i++;
    } else if (strcmp(arg, "--benchmark") == 0) {
      if (leer_entero("--benchmark", siguiente, 1, 1000000, &valor) != 0)
        return -1;
      cfg->frames_benchmark = (int)valor;
      i++;
    } else if (arg[0] == '-' && arg[1] == '-') {
      fprintf(stderr, "Error: opcion desconocida '%s'\n", arg);
      return -1;
    } else if (!n_recibido) {
      /* Primer argumento posicional: N */
      if (leer_entero("N", arg, 1, N_MAXIMO, &valor) != 0)
        return -1;
      cfg->num_particulas = (int)valor;
      n_recibido = 1;
    } else {
      fprintf(stderr, "Error: argumento inesperado '%s'\n", arg);
      return -1;
    }
  }

  if (!n_recibido) {
    fprintf(stderr, "Error: falta el parametro N (cantidad de particulas)\n");
    return -1;
  }

#ifndef _OPENMP
  if (cfg->modo != MODO_SECUENCIAL) {
    fprintf(stderr, "Aviso: compilado sin OpenMP, se usara el modo "
                    "secuencial\n");
    cfg->modo = MODO_SECUENCIAL;
  }
#endif
  if (cfg->modo == MODO_SECUENCIAL) {
    cfg->num_hilos = 1;
  }
  return 0;
}
