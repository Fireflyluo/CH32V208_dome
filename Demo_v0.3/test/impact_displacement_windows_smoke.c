#include "impact_displacement.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 2048u
#define DEFAULT_CSV_PATH "lib/impact_displacement/test/data/ideal_collision_event.csv"

static int load_csv_samples(const char *path, impact_disp_sample_t *samples,
                            uint32_t *count_out) {
  FILE *fp;
  char line[256];
  uint32_t count = 0u;

  if (path == NULL || samples == NULL || count_out == NULL) {
    return -1;
  }

  fp = fopen(path, "r");
  if (fp == NULL) {
    return -2;
  }

  if (fgets(line, sizeof(line), fp) == NULL) {
    fclose(fp);
    return -3;
  }

  while (fgets(line, sizeof(line), fp) != NULL) {
    unsigned long ts = 0ul;
    int ax = 0;
    int ay = 0;
    int az = 0;
    int flags = 0;

    if (count >= MAX_SAMPLES) {
      fclose(fp);
      return -4;
    }

    if (sscanf(line, "%lu,%d,%d,%d,%d", &ts, &ax, &ay, &az, &flags) != 5) {
      fclose(fp);
      return -5;
    }

    samples[count].timestamp_us = (uint32_t)ts;
    samples[count].ax_mg = (int16_t)ax;
    samples[count].ay_mg = (int16_t)ay;
    samples[count].az_mg = (int16_t)az;
    samples[count].flags = (uint8_t)flags;
    samples[count].reserved[0] = 0u;
    samples[count].reserved[1] = 0u;
    samples[count].reserved[2] = 0u;
    count++;
  }

  fclose(fp);
  *count_out = count;
  return 0;
}

int main(int argc, char **argv) {
  impact_disp_ctx_t ctx;
  impact_disp_cfg_t cfg;
  impact_disp_result_t result;
  impact_disp_sample_t samples[MAX_SAMPLES];
  uint32_t sample_count = 0u;
  const char *csv_path = DEFAULT_CSV_PATH;
  uint16_t pre_samples = 40u;
  int strict_ideal_check = 1;
  int rc;
  impact_disp_status_t st;
  const float expected_dx_mm = 10.0f;

  if (argc >= 2 && argv[1] != NULL && argv[1][0] != '\0') {
    csv_path = argv[1];
    strict_ideal_check = 0;
  }
  if (argc >= 3 && argv[2] != NULL) {
    long parsed = strtol(argv[2], NULL, 10);
    if (parsed <= 0 || parsed > 65535) {
      printf("invalid pre_samples: %s\n", argv[2]);
      return 1;
    }
    pre_samples = (uint16_t)parsed;
    strict_ideal_check = 0;
  }

  rc = load_csv_samples(csv_path, samples, &sample_count);
  if (rc != 0) {
    printf("load_csv_samples failed: %d\n", rc);
    return 1;
  }

  impact_disp_get_default_cfg(&cfg);
  cfg.sample_rate_hz = 400u;
  cfg.min_event_ms = 100u;
  cfg.max_event_ms = 1000u;
  cfg.release_threshold_mg = 80u;
  cfg.release_count_min = 10u;

  st = impact_disp_init(&ctx, &cfg);
  if (st != IMPACT_DISP_OK) {
    printf("impact_disp_init failed: %d\n", (int)st);
    return 1;
  }

  st = impact_disp_process_event(&ctx, samples, sample_count, pre_samples, 1u,
                                 &result);
  if (st != IMPACT_DISP_OK) {
    printf("impact_disp_process_event failed: %d\n", (int)st);
    return 1;
  }

  printf("csv=%s pre_samples=%u\n", csv_path, (unsigned)pre_samples);
  printf("impact_displacement version: %s\n", impact_disp_get_version());
  printf("samples=%lu duration_ms=%lu\n", (unsigned long)result.sample_count,
         (unsigned long)result.duration_ms);
  printf("dx=%.3f mm dy=%.3f mm dz=%.3f mm disp=%.3f mm\n", result.dx_mm,
         result.dy_mm, result.dz_mm, result.disp_mm);
  printf("peak_acc=%.3f mg terminal_speed=%.3f mm/s confidence=%u flags=0x%08lX\n",
         result.peak_acc_mg, result.terminal_speed_mm_s,
         (unsigned)result.confidence, (unsigned long)result.quality_flags);

  if (strict_ideal_check != 0) {
    if (fabsf(result.dx_mm - expected_dx_mm) > 1.0f) {
      printf("dx check failed: expected %.3f mm, got %.3f mm\n", expected_dx_mm,
             result.dx_mm);
      return 1;
    }
    if (fabsf(result.dy_mm) > 0.5f || fabsf(result.dz_mm) > 0.5f) {
      printf("dy/dz check failed\n");
      return 1;
    }
    if (result.confidence < 80u) {
      printf("confidence too low: %u\n", (unsigned)result.confidence);
      return 1;
    }
  }

  printf("impact_displacement_windows_smoke: PASS\n");
  return 0;
}
