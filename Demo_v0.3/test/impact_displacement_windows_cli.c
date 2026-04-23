#include "impact_displacement.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 4096u

typedef struct {
  const char *csv_path;
  uint16_t pre_samples;
  uint16_t sample_rate_hz; /* 0 means auto-estimate from timestamp delta */
  uint16_t min_event_ms;
  uint16_t max_event_ms;
  uint16_t release_threshold_mg;
  uint16_t release_count_min;
  uint16_t max_dt_ms;
  uint16_t gravity_ema_tau_ms;
  uint32_t event_id;
} cli_opts_t;

static void print_usage(const char *exe) {
  printf("usage: %s <csv_path> [options]\n", exe);
  printf("options:\n");
  printf("  --pre <N>               pre_samples (default: 30)\n");
  printf("  --rate <Hz>             sample_rate_hz, 0=auto (default: 0)\n");
  printf("  --min-ms <N>            min_event_ms (default: 80)\n");
  printf("  --max-ms <N>            max_event_ms (default: 20000)\n");
  printf("  --release-thr <mg>      release_threshold_mg (default: 80)\n");
  printf("  --release-count <N>     release_count_min (default: 5)\n");
  printf("  --max-dt-ms <N>         max_dt_ms (default: 500)\n");
  printf("  --grav-ema-ms <N>       gravity EMA time constant in ms, 0=off (default: 0)\n");
  printf("  --event-id <N>          event id (default: 1)\n");
}

static int parse_u16(const char *s, uint16_t *out) {
  unsigned long v = 0ul;
  char *end = NULL;
  if (s == NULL || out == NULL) {
    return -1;
  }
  v = strtoul(s, &end, 10);
  if (end == s || *end != '\0' || v > 65535ul) {
    return -1;
  }
  *out = (uint16_t)v;
  return 0;
}

static int parse_u32(const char *s, uint32_t *out) {
  unsigned long v = 0ul;
  char *end = NULL;
  if (s == NULL || out == NULL) {
    return -1;
  }
  v = strtoul(s, &end, 10);
  if (end == s || *end != '\0') {
    return -1;
  }
  *out = (uint32_t)v;
  return 0;
}

static int load_csv_samples(const char *path, impact_disp_sample_t *samples,
                            uint32_t *count_out) {
  FILE *fp = NULL;
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

static uint16_t estimate_rate_hz(const impact_disp_sample_t *samples,
                                 uint32_t count) {
  uint64_t dt_sum = 0ull;
  uint32_t dt_cnt = 0u;
  uint32_t i;
  if (samples == NULL || count < 2u) {
    return 0u;
  }
  for (i = 1u; i < count; ++i) {
    uint32_t t0 = samples[i - 1u].timestamp_us;
    uint32_t t1 = samples[i].timestamp_us;
    if (t1 > t0) {
      dt_sum += (uint64_t)(t1 - t0);
      dt_cnt++;
    }
  }
  if (dt_cnt == 0u || dt_sum == 0ull) {
    return 0u;
  }
  {
    uint32_t avg_dt_us = (uint32_t)(dt_sum / (uint64_t)dt_cnt);
    uint32_t hz = (avg_dt_us > 0u) ? ((1000000u + (avg_dt_us / 2u)) / avg_dt_us) : 0u;
    if (hz == 0u) {
      hz = 1u;
    }
    if (hz > 2000u) {
      hz = 2000u;
    }
    return (uint16_t)hz;
  }
}

static void print_flags(uint32_t flags) {
  printf("quality_flags=0x%08lX\n", (unsigned long)flags);
  if (flags == 0u) {
    printf("  - none\n");
    return;
  }
  if (flags & IMPACT_DISP_QF_TS_NON_MONOTONIC) printf("  - TS_NON_MONOTONIC\n");
  if (flags & IMPACT_DISP_QF_DT_GAP) printf("  - DT_GAP\n");
  if (flags & IMPACT_DISP_QF_CLIPPED) printf("  - CLIPPED\n");
  if (flags & IMPACT_DISP_QF_NO_RELEASE) printf("  - NO_RELEASE\n");
  if (flags & IMPACT_DISP_QF_ROTATION_HIGH) printf("  - ROTATION_HIGH\n");
  if (flags & IMPACT_DISP_QF_LOW_SAMPLE_COUNT) printf("  - LOW_SAMPLE_COUNT\n");
  if (flags & IMPACT_DISP_QF_DURATION_TOO_SHORT) printf("  - DURATION_TOO_SHORT\n");
}

int main(int argc, char **argv) {
  impact_disp_sample_t samples[MAX_SAMPLES];
  impact_disp_ctx_t ctx;
  impact_disp_cfg_t cfg;
  impact_disp_result_t out;
  cli_opts_t opts;
  uint32_t sample_count = 0u;
  int rc;
  impact_disp_status_t st;
  int i;

  memset(&opts, 0, sizeof(opts));
  opts.pre_samples = 30u;
  opts.sample_rate_hz = 0u;
  opts.min_event_ms = 80u;
  opts.max_event_ms = 20000u;
  opts.release_threshold_mg = 80u;
  opts.release_count_min = 5u;
  opts.max_dt_ms = 500u;
  opts.gravity_ema_tau_ms = 0u;
  opts.event_id = 1u;

  if (argc < 2) {
    print_usage((argc > 0 && argv[0] != NULL) ? argv[0] : "impact_displacement_windows_cli");
    return 1;
  }
  opts.csv_path = argv[1];

  for (i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "--pre") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.pre_samples) != 0) return 1;
    } else if (strcmp(argv[i], "--rate") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.sample_rate_hz) != 0) return 1;
    } else if (strcmp(argv[i], "--min-ms") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.min_event_ms) != 0) return 1;
    } else if (strcmp(argv[i], "--max-ms") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.max_event_ms) != 0) return 1;
    } else if (strcmp(argv[i], "--release-thr") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.release_threshold_mg) != 0) return 1;
    } else if (strcmp(argv[i], "--release-count") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.release_count_min) != 0) return 1;
    } else if (strcmp(argv[i], "--max-dt-ms") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.max_dt_ms) != 0) return 1;
    } else if (strcmp(argv[i], "--grav-ema-ms") == 0 && (i + 1) < argc) {
      if (parse_u16(argv[++i], &opts.gravity_ema_tau_ms) != 0) return 1;
    } else if (strcmp(argv[i], "--event-id") == 0 && (i + 1) < argc) {
      if (parse_u32(argv[++i], &opts.event_id) != 0) return 1;
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  rc = load_csv_samples(opts.csv_path, samples, &sample_count);
  if (rc != 0) {
    printf("load_csv_samples failed: %d\n", rc);
    return 1;
  }

  impact_disp_get_default_cfg(&cfg);
  if (opts.sample_rate_hz == 0u) {
    opts.sample_rate_hz = estimate_rate_hz(samples, sample_count);
    if (opts.sample_rate_hz == 0u) {
      opts.sample_rate_hz = 10u;
    }
  }
  cfg.sample_rate_hz = opts.sample_rate_hz;
  cfg.min_event_ms = opts.min_event_ms;
  cfg.max_event_ms = opts.max_event_ms;
  cfg.release_threshold_mg = opts.release_threshold_mg;
  cfg.release_count_min = opts.release_count_min;
  cfg.max_dt_ms = opts.max_dt_ms;
  cfg.gravity_ema_tau_ms = opts.gravity_ema_tau_ms;

  st = impact_disp_init(&ctx, &cfg);
  if (st != IMPACT_DISP_OK) {
    printf("impact_disp_init failed: %d\n", (int)st);
    return 1;
  }

  st = impact_disp_process_event(&ctx, samples, sample_count, opts.pre_samples,
                                 opts.event_id, &out);
  if (st != IMPACT_DISP_OK) {
    printf("impact_disp_process_event failed: %d\n", (int)st);
    return 1;
  }

  printf("impact_displacement version: %s\n", impact_disp_get_version());
  printf("csv=%s\n", opts.csv_path);
  printf("sample_count=%lu pre_samples=%u\n", (unsigned long)sample_count,
         (unsigned)opts.pre_samples);
  printf("cfg: rate=%uHz min_ms=%u max_ms=%u release_thr=%u release_cnt=%u max_dt_ms=%u grav_ema_ms=%u\n",
         (unsigned)cfg.sample_rate_hz, (unsigned)cfg.min_event_ms,
         (unsigned)cfg.max_event_ms, (unsigned)cfg.release_threshold_mg,
         (unsigned)cfg.release_count_min, (unsigned)cfg.max_dt_ms,
         (unsigned)cfg.gravity_ema_tau_ms);
  printf("result: duration_ms=%lu samples=%lu dx=%.3f dy=%.3f dz=%.3f disp=%.3f\n",
         (unsigned long)out.duration_ms, (unsigned long)out.sample_count,
         out.dx_mm, out.dy_mm, out.dz_mm, out.disp_mm);
  printf("result: peak=%.3fmg terminal=%.3fmm/s rot_err=%.3fmg confidence=%u\n",
         out.peak_acc_mg, out.terminal_speed_mm_s, out.rotation_error_mg,
         (unsigned)out.confidence);
  print_flags(out.quality_flags);

  return 0;
}
