#include "impact_displacement.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 4096u

typedef struct {
  const char *name;
  const char *csv_path;
  uint16_t pre_samples;
  float dx_min;
  float dx_max;
  float dy_min;
  float dy_max;
  float dz_min;
  float dz_max;
  float disp_min;
  float disp_max;
  uint8_t min_confidence;
  uint32_t required_clear_flags;
} regression_case_t;

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

static int check_range(const char *label, float value, float min_v, float max_v) {
  if (value < min_v || value > max_v) {
    printf("  FAIL %s: value=%.3f expected=[%.3f, %.3f]\n", label, value, min_v,
           max_v);
    return 1;
  }
  return 0;
}

static int run_case(const regression_case_t *tc) {
  impact_disp_sample_t samples[MAX_SAMPLES];
  impact_disp_cfg_t cfg;
  impact_disp_ctx_t ctx;
  impact_disp_result_t out;
  uint32_t sample_count = 0u;
  int rc;
  int fails = 0;
  impact_disp_status_t st;

  rc = load_csv_samples(tc->csv_path, samples, &sample_count);
  if (rc != 0) {
    printf("[%s] FAIL load_csv_samples rc=%d\n", tc->name, rc);
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
    printf("[%s] FAIL impact_disp_init st=%d\n", tc->name, (int)st);
    return 1;
  }

  st = impact_disp_process_event(&ctx, samples, sample_count, tc->pre_samples,
                                 100u, &out);
  if (st != IMPACT_DISP_OK) {
    printf("[%s] FAIL impact_disp_process_event st=%d\n", tc->name, (int)st);
    return 1;
  }

  printf("[%s] samples=%lu duration_ms=%lu dx=%.3f dy=%.3f dz=%.3f disp=%.3f "
         "peak=%.3f conf=%u flags=0x%08lX\n",
         tc->name, (unsigned long)out.sample_count, (unsigned long)out.duration_ms,
         out.dx_mm, out.dy_mm, out.dz_mm, out.disp_mm, out.peak_acc_mg,
         (unsigned)out.confidence, (unsigned long)out.quality_flags);

  fails += check_range("dx_mm", out.dx_mm, tc->dx_min, tc->dx_max);
  fails += check_range("dy_mm", out.dy_mm, tc->dy_min, tc->dy_max);
  fails += check_range("dz_mm", out.dz_mm, tc->dz_min, tc->dz_max);
  fails += check_range("disp_mm", out.disp_mm, tc->disp_min, tc->disp_max);

  if (out.confidence < tc->min_confidence) {
    printf("  FAIL confidence: value=%u expected>=%u\n", (unsigned)out.confidence,
           (unsigned)tc->min_confidence);
    fails++;
  }

  if ((out.quality_flags & tc->required_clear_flags) != 0u) {
    printf("  FAIL quality_flags: got=0x%08lX clear_mask=0x%08lX\n",
           (unsigned long)out.quality_flags,
           (unsigned long)tc->required_clear_flags);
    fails++;
  }

  if (fails == 0) {
    printf("[%s] PASS\n", tc->name);
    return 0;
  }

  printf("[%s] FAIL count=%d\n", tc->name, fails);
  return 1;
}

int main(void) {
  static const regression_case_t cases[] = {
      {
          "ideal_collision",
          "lib/impact_displacement/test/data/ideal_collision_event.csv",
          40u,
          9.0f,
          11.0f,
          -0.5f,
          0.5f,
          -0.5f,
          0.5f,
          9.0f,
          11.0f,
          95u,
          IMPACT_DISP_QF_TS_NON_MONOTONIC | IMPACT_DISP_QF_DT_GAP |
              IMPACT_DISP_QF_CLIPPED | IMPACT_DISP_QF_NO_RELEASE |
              IMPACT_DISP_QF_ROTATION_HIGH | IMPACT_DISP_QF_LOW_SAMPLE_COUNT |
              IMPACT_DISP_QF_DURATION_TOO_SHORT,
      },
      {
          "nonideal_collision",
          "lib/impact_displacement/test/data/nonideal_collision_event.csv",
          60u,
          11.0f,
          15.0f,
          0.5f,
          3.0f,
          6.0f,
          10.5f,
          13.0f,
          18.0f,
          90u,
          IMPACT_DISP_QF_TS_NON_MONOTONIC | IMPACT_DISP_QF_DT_GAP |
              IMPACT_DISP_QF_CLIPPED | IMPACT_DISP_QF_NO_RELEASE |
              IMPACT_DISP_QF_ROTATION_HIGH | IMPACT_DISP_QF_LOW_SAMPLE_COUNT |
              IMPACT_DISP_QF_DURATION_TOO_SHORT,
      },
  };

  size_t i;
  int failed = 0;

  printf("impact_displacement version: %s\n", impact_disp_get_version());

  for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    failed += run_case(&cases[i]);
  }

  if (failed != 0) {
    printf("impact_displacement_windows_regression: FAIL (%d cases)\n", failed);
    return 1;
  }

  printf("impact_displacement_windows_regression: PASS\n");
  return 0;
}
