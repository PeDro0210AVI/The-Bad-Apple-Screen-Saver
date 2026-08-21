#include "frames.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAP_MAGIC "BADA"

Frames *frames_load(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    fprintf(stderr, "frames_load: no se pudo abrir '%s'\n", path);
    return NULL;
  }

  char magic[4];
  if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, BAP_MAGIC, 4) != 0) {
    fprintf(stderr, "frames_load: '%s' no tiene el encabezado BADA\n", path);
    fclose(fp);
    return NULL;
  }

  uint32_t cols, rows, frame_count;
  float fps;
  if (fread(&cols, sizeof(cols), 1, fp) != 1 ||
      fread(&rows, sizeof(rows), 1, fp) != 1 ||
      fread(&frame_count, sizeof(frame_count), 1, fp) != 1 ||
      fread(&fps, sizeof(fps), 1, fp) != 1) {
    fprintf(stderr, "frames_load: encabezado incompleto en '%s'\n", path);
    fclose(fp);
    return NULL;
  }

  if (cols == 0 || rows == 0 || frame_count == 0) {
    fprintf(stderr, "frames_load: dimensiones invalidas en '%s'\n", path);
    fclose(fp);
    return NULL;
  }

  size_t row_bytes = (cols + 7) / 8;
  size_t frame_bytes = row_bytes * rows;
  size_t total_bytes = frame_bytes * (size_t)frame_count;

  uint8_t *data = malloc(total_bytes);
  if (!data) {
    fprintf(stderr, "frames_load: sin memoria para %zu bytes\n", total_bytes);
    fclose(fp);
    return NULL;
  }

  if (fread(data, 1, total_bytes, fp) != total_bytes) {
    fprintf(stderr, "frames_load: '%s' truncado (se esperaban %zu bytes)\n",
            path, total_bytes);
    free(data);
    fclose(fp);
    return NULL;
  }
  fclose(fp);

  Frames *frames = malloc(sizeof(Frames));
  if (!frames) {
    free(data);
    return NULL;
  }

  frames->cols = cols;
  frames->rows = rows;
  frames->frame_count = frame_count;
  frames->fps = fps;
  frames->data = data;
  return frames;
}

void frames_free(Frames *frames) {
  if (!frames) {
    return;
  }
  free(frames->data);
  free(frames);
}

int frames_get_pixel(const Frames *frames, uint32_t frame_idx, uint32_t x,
                      uint32_t y) {
  size_t row_bytes = (frames->cols + 7) / 8;
  size_t frame_bytes = row_bytes * frames->rows;
  const uint8_t *frame = frames->data + (size_t)frame_idx * frame_bytes;

  size_t byte_idx = y * row_bytes + x / 8;
  uint8_t bit_mask = 0x80 >> (x % 8);
  return (frame[byte_idx] & bit_mask) != 0;
}
