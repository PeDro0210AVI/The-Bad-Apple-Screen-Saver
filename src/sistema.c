#include "sistema.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define VELOCIDAD_INICIAL_MIN 40.0f
#define VELOCIDAD_INICIAL_MAX 160.0f

/* Generador pseudoaleatorio xorshift32: portable (Linux, macOS, Windows) y
 * reproducible con la misma semilla, para que las versiones secuencial y
 * paralela simulen exactamente la misma escena. */
static uint32_t estado_rng = 2463534242u;

static void rng_sembrar(unsigned int semilla) {
  estado_rng = semilla ? semilla : 2463534242u;
}

static float rng_uniforme(void) {
  estado_rng ^= estado_rng << 13;
  estado_rng ^= estado_rng >> 17;
  estado_rng ^= estado_rng << 5;
  return (estado_rng >> 8) * (1.0f / 16777216.0f); /* [0, 1) */
}

/* Convierte un color HSV (h en [0,1)) a RGB. Se usa saturacion y brillo
 * altos para que los colores aleatorios sean vivos. */
static void hsv_a_rgb(float h, float s, float v, float *r, float *g,
                      float *b) {
  float h6 = h * 6.0f;
  int sector = (int)h6 % 6;
  float f = h6 - floorf(h6);
  float p = v * (1.0f - s);
  float q = v * (1.0f - s * f);
  float t = v * (1.0f - s * (1.0f - f));
  switch (sector) {
  case 0: *r = v; *g = t; *b = p; break;
  case 1: *r = q; *g = v; *b = p; break;
  case 2: *r = p; *g = v; *b = t; break;
  case 3: *r = p; *g = q; *b = v; break;
  case 4: *r = t; *g = p; *b = v; break;
  default: *r = v; *g = p; *b = q; break;
  }
}

Sistema *sistema_crear(const Config *cfg) {
  Sistema *s = calloc(1, sizeof(Sistema));
  if (!s) {
    return NULL;
  }
  int n = cfg->num_particulas;
  s->n = n;
  s->ancho = (float)cfg->ancho;
  s->alto = (float)cfg->alto;
  s->radio = cfg->radio;
  s->frame_silueta = -1;

  size_t bytes = (size_t)n * sizeof(float);
  s->x = malloc(bytes);
  s->y = malloc(bytes);
  s->vx = malloc(bytes);
  s->vy = malloc(bytes);
  s->x_sig = malloc(bytes);
  s->y_sig = malloc(bytes);
  s->vx_sig = malloc(bytes);
  s->vy_sig = malloc(bytes);
  s->color_base = malloc(3 * bytes);
  s->vertices = malloc(2 * bytes);
  s->colores = malloc(3 * bytes);
  if (!s->x || !s->y || !s->vx || !s->vy || !s->x_sig || !s->y_sig ||
      !s->vx_sig || !s->vy_sig || !s->color_base || !s->vertices ||
      !s->colores) {
    fprintf(stderr, "sistema_crear: sin memoria para %d particulas\n", n);
    sistema_destruir(s);
    return NULL;
  }

  /* Posicion, direccion (angulo con sin/cos) y color pseudoaleatorios */
  rng_sembrar(cfg->semilla);
  for (int i = 0; i < n; i++) {
    s->x[i] = s->radio + rng_uniforme() * (s->ancho - 2.0f * s->radio);
    s->y[i] = s->radio + rng_uniforme() * (s->alto - 2.0f * s->radio);
    float angulo = rng_uniforme() * 2.0f * (float)M_PI;
    float rapidez = VELOCIDAD_INICIAL_MIN +
                    rng_uniforme() *
                        (VELOCIDAD_INICIAL_MAX - VELOCIDAD_INICIAL_MIN);
    s->vx[i] = cosf(angulo) * rapidez;
    s->vy[i] = sinf(angulo) * rapidez;
    hsv_a_rgb(rng_uniforme(), 0.75f + 0.25f * rng_uniforme(), 1.0f,
              &s->color_base[3 * i], &s->color_base[3 * i + 1],
              &s->color_base[3 * i + 2]);
    s->vertices[2 * i] = s->x[i];
    s->vertices[2 * i + 1] = s->y[i];
    s->colores[3 * i] = s->color_base[3 * i];
    s->colores[3 * i + 1] = s->color_base[3 * i + 1];
    s->colores[3 * i + 2] = s->color_base[3 * i + 2];
  }

  /* La silueta es opcional: si no hay archivo .bap el screensaver sigue
   * funcionando con movimiento circular y rebotes. */
  s->frames = frames_load(cfg->ruta_bap);
  if (s->frames) {
    size_t celdas = (size_t)s->frames->cols * s->frames->rows;
    s->pix_x = malloc(celdas * sizeof(float));
    s->pix_y = malloc(celdas * sizeof(float));
    s->fondo = malloc(celdas);
    if (!s->pix_x || !s->pix_y || !s->fondo) {
      fprintf(stderr, "sistema_crear: sin memoria para la silueta\n");
      sistema_destruir(s);
      return NULL;
    }
  } else {
    fprintf(stderr, "Aviso: sin silueta Bad Apple, modo solo rebotes\n");
  }
  return s;
}

void sistema_destruir(Sistema *s) {
  if (!s) {
    return;
  }
  free(s->x);
  free(s->y);
  free(s->vx);
  free(s->vy);
  free(s->x_sig);
  free(s->y_sig);
  free(s->vx_sig);
  free(s->vy_sig);
  free(s->color_base);
  free(s->vertices);
  free(s->colores);
  free(s->pix_x);
  free(s->pix_y);
  free(s->fondo);
  frames_free(s->frames);
  free(s);
}

void sistema_actualizar_silueta(Sistema *s, double t) {
  if (!s->frames) {
    return;
  }
  const Frames *f = s->frames;
  float fps = f->fps > 0.0f ? f->fps : 30.0f;
  int64_t idx = (int64_t)(t * fps) % f->frame_count;
  if (idx == s->frame_silueta) {
    return; /* el frame del video no cambio */
  }
  s->frame_silueta = idx;

  /* Cada celda encendida de la grilla se convierte en un punto destino en
   * coordenadas de pantalla (la fila 0 del video es la de arriba). */
  float celda_w = s->ancho / (float)f->cols;
  float celda_h = s->alto / (float)f->rows;
  int cuenta = 0;
  for (uint32_t y = 0; y < f->rows; y++) {
    for (uint32_t x = 0; x < f->cols; x++) {
      int encendido = frames_get_pixel(f, (uint32_t)idx, x, y);
      /* OpenGL dibuja desde abajo: se invierte la fila para el fondo */
      s->fondo[(f->rows - 1 - y) * f->cols + x] = encendido ? 45 : 0;
      if (encendido) {
        s->pix_x[cuenta] = (x + 0.5f) * celda_w;
        s->pix_y[cuenta] = s->alto - (y + 0.5f) * celda_h;
        cuenta++;
      }
    }
  }
  s->num_pix = cuenta;
}

void sistema_intercambiar_buffers(Sistema *s) {
  float *tmp;
  tmp = s->x; s->x = s->x_sig; s->x_sig = tmp;
  tmp = s->y; s->y = s->y_sig; s->y_sig = tmp;
  tmp = s->vx; s->vx = s->vx_sig; s->vx_sig = tmp;
  tmp = s->vy; s->vy = s->vy_sig; s->vy_sig = tmp;
}
