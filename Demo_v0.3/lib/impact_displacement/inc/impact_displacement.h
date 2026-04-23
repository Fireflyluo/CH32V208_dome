#ifndef IMPACT_DISPLACEMENT_H
#define IMPACT_DISPLACEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
  IMPACT_DISP_OK = 0,
  IMPACT_DISP_ERR_ARG = -1,
  IMPACT_DISP_ERR_STATE = -2,
  IMPACT_DISP_ERR_NOT_READY = -3
} impact_disp_status_t;

typedef enum {
  IMPACT_DISP_STATE_UNINIT = 0,
  IMPACT_DISP_STATE_IDLE = 1,
  IMPACT_DISP_STATE_COLLECTING = 2,
  IMPACT_DISP_STATE_FINISHED = 3,
  IMPACT_DISP_STATE_ERROR = 4
} impact_disp_state_t;

enum {
  IMPACT_DISP_QF_NONE = 0u,
  IMPACT_DISP_QF_TS_NON_MONOTONIC = (1u << 0),
  IMPACT_DISP_QF_DT_GAP = (1u << 1),
  IMPACT_DISP_QF_CLIPPED = (1u << 2),
  IMPACT_DISP_QF_NO_RELEASE = (1u << 3),
  IMPACT_DISP_QF_ROTATION_HIGH = (1u << 4),
  IMPACT_DISP_QF_LOW_SAMPLE_COUNT = (1u << 5),
  IMPACT_DISP_QF_DURATION_TOO_SHORT = (1u << 6)
};

enum {
  IMPACT_DISP_SAMPLE_CLIPPED = (1u << 0)
};

typedef struct {
  uint16_t sample_rate_hz;
  uint16_t min_event_ms;
  uint16_t max_event_ms;
  uint16_t release_threshold_mg;
  uint16_t release_count_min;
  uint16_t max_dt_ms;
  float rotation_limit_deg;
  /* 0 disables. When enabled, estimate gravity vector with EMA (low-pass) and
   * subtract it to get dynamic acceleration. Useful to reduce gravity leakage
   * from small tilts during longer/slow motions on PC validation. */
  uint16_t gravity_ema_tau_ms;
  uint8_t enable_zero_velocity_correction;
  uint8_t reserved[1];
} impact_disp_cfg_t;

typedef struct {
  uint32_t timestamp_us;
  int16_t ax_mg;
  int16_t ay_mg;
  int16_t az_mg;
  uint8_t flags;
  uint8_t reserved[3];
} impact_disp_sample_t;

typedef struct {
  uint32_t event_id;
  uint32_t duration_ms;
  uint32_t sample_count;
  float dx_mm;
  float dy_mm;
  float dz_mm;
  float disp_mm;
  float peak_acc_mg;
  float terminal_speed_mm_s;
  float rotation_error_mg;
  uint8_t confidence;
  uint8_t reserved[3];
  uint32_t quality_flags;
} impact_disp_result_t;

typedef struct {
  impact_disp_cfg_t cfg;
  impact_disp_state_t state;
  uint32_t event_id;
  uint32_t quality_flags;
  uint32_t first_ts_us;
  uint32_t last_ts_us;
  uint32_t sample_count;
  uint16_t release_count;
  uint16_t reserved0;

  float baseline_ax_mg;
  float baseline_ay_mg;
  float baseline_az_mg;
  float g_hat_ax_mg;
  float g_hat_ay_mg;
  float g_hat_az_mg;
  uint8_t baseline_set;
  uint8_t g_hat_set;
  uint8_t has_prev;
  uint8_t reserved1[1];

  float prev_ax_mps2;
  float prev_ay_mps2;
  float prev_az_mps2;

  float vx_mps;
  float vy_mps;
  float vz_mps;
  float x_m;
  float y_m;
  float z_m;

  float peak_acc_mg;
  float tail_sum_ax_mg;
  float tail_sum_ay_mg;
  float tail_sum_az_mg;
  uint32_t tail_count;
} impact_disp_ctx_t;

const char *impact_disp_get_version(void);
void impact_disp_get_default_cfg(impact_disp_cfg_t *cfg);

impact_disp_status_t impact_disp_init(impact_disp_ctx_t *ctx,
                                      const impact_disp_cfg_t *cfg);
impact_disp_status_t impact_disp_reset(impact_disp_ctx_t *ctx);
impact_disp_status_t impact_disp_begin_event(impact_disp_ctx_t *ctx,
                                             uint32_t event_id);
impact_disp_status_t impact_disp_set_baseline_mg(impact_disp_ctx_t *ctx,
                                                 float bx_mg, float by_mg,
                                                 float bz_mg);
impact_disp_status_t impact_disp_feed_sample(impact_disp_ctx_t *ctx,
                                             const impact_disp_sample_t *sample);
impact_disp_status_t impact_disp_end_event(impact_disp_ctx_t *ctx,
                                           impact_disp_result_t *out);
impact_disp_status_t impact_disp_process_event(impact_disp_ctx_t *ctx,
                                               const impact_disp_sample_t *samples,
                                               uint32_t sample_count,
                                               uint16_t pre_samples,
                                               uint32_t event_id,
                                               impact_disp_result_t *out);
impact_disp_state_t impact_disp_get_state(const impact_disp_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* IMPACT_DISPLACEMENT_H */
