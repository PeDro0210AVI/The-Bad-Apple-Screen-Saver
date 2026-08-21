/* ------------------------------------------------------------------------
 * Loader del formato .bap (Bad Apple Pattern): la salida de
 * scripts/extract_frames.py. Cada frame es una grilla cols x rows de bits
 * (1 = silueta, 0 = fondo), empacada 8 pixeles por byte.
 * -------------------------------------------------------------------------*/

#ifndef FRAMES_H
#define FRAMES_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint32_t cols;
  uint32_t rows;
  uint32_t frame_count;
  float fps;
  uint8_t *data; /* frame_count * row_bytes * rows, contiguo */
} Frames;

/* Carga un archivo .bap completo en memoria. Retorna NULL si el archivo
 * no existe, no se puede leer o su encabezado es invalido. */
Frames *frames_load(const char *path);

/* Libera la memoria reservada por frames_load. */
void frames_free(Frames *frames);

/* Retorna 1 si el pixel (x, y) del frame_idx pertenece a la silueta,
 * 0 en caso contrario. No valida limites: x < cols, y < rows,
 * frame_idx < frame_count deben cumplirse. */
int frames_get_pixel(const Frames *frames, uint32_t frame_idx, uint32_t x,
                      uint32_t y);

#endif /* FRAMES_H */
