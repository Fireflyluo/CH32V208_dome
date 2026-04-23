#include "impact_displacement.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define IMPACT_DISP_VERSION "0.1.1"

#define GRAVITY_MPS2 9.80665f
#define MG_TO_MPS2 (GRAVITY_MPS2 / 1000.0f)
#define MPS_TO_MMPS 1000.0f
#define M_TO_MM 1000.0f
#define DEG_TO_RAD 0.01745329251994329577f

static float impact_disp_norm3(float x, float y, float z) {
  return sqrtf((x * x) + (y * y) + (z * z));
}

static uint8_t impact_disp_clamp_u8(int v) {
  if (v < 0) {
    return 0u;
  }
  if (v > 100) {
    return 100u;
  }
  return (uint8_t)v;
}

static uint32_t impact_disp_nominal_dt_us(const impact_disp_cfg_t *cfg) {
  if (cfg == NULL || cfg->sample_rate_hz == 0u) {
    return 0u;
  }
  return (uint32_t)(1000000u / cfg->sample_rate_hz);
}

static float impact_disp_rotation_limit_mg(float limit_deg) {
  float half_rad = 0.5f * limit_deg * DEG_TO_RAD;
  return 2000.0f * sinf(half_rad);
}

static int impact_disp_confidence_from_flags(uint32_t flags, uint32_t samples,
                                             uint16_t release_count,
                                             uint16_t release_count_min) {
  int score = 100;

  if (samples < 3u) {
    score -= 45;
  }
  if (release_count < release_count_min) {
    score -= 25;
  }
  if ((flags & IMPACT_DISP_QF_ROTATION_HIGH) != 0u) {
    score -= 35;
  }
  if ((flags & IMPACT_DISP_QF_DT_GAP) != 0u) {
    score -= 10;
  }
  if ((flags & IMPACT_DISP_QF_CLIPPED) != 0u) {
    score -= 10;
  }
  if ((flags & IMPACT_DISP_QF_TS_NON_MONOTONIC) != 0u) {
    score -= 20;
  }
  if ((flags & IMPACT_DISP_QF_DURATION_TOO_SHORT) != 0u) {
    score -= 20;
  }

  return score;
}

const char *impact_disp_get_version(void) { return IMPACT_DISP_VERSION; }

void impact_disp_get_default_cfg(impact_disp_cfg_t *cfg) {
  if (cfg == NULL) {
    return;
  }

  memset(cfg, 0, sizeof(*cfg));
  cfg->sample_rate_hz = 400u;
  cfg->min_event_ms = 40u;
  cfg->max_event_ms = 1000u;
  cfg->release_threshold_mg = 80u;
  cfg->release_count_min = 8u;
  cfg->max_dt_ms = 20u;
  cfg->rotation_limit_deg = 15.0f;
  cfg->gravity_ema_tau_ms = 0u;
  cfg->enable_zero_velocity_correction = 1u;
}

impact_disp_status_t impact_disp_reset(impact_disp_ctx_t *ctx) {
  impact_disp_cfg_t backup_cfg;
  uint8_t had_cfg;

  if (ctx == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }

  had_cfg = (ctx->cfg.sample_rate_hz != 0u) ? 1u : 0u;
  backup_cfg = ctx->cfg;
  memset(ctx, 0, sizeof(*ctx));

  if (had_cfg != 0u) {
    ctx->cfg = backup_cfg;
  } else {
    impact_disp_get_default_cfg(&ctx->cfg);
  }
  ctx->state = IMPACT_DISP_STATE_IDLE;
  return IMPACT_DISP_OK;
}

impact_disp_status_t impact_disp_init(impact_disp_ctx_t *ctx,
                                      const impact_disp_cfg_t *cfg) {
  if (ctx == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }

  memset(ctx, 0, sizeof(*ctx));
  if (cfg != NULL) {
    ctx->cfg = *cfg;
  } else {
    impact_disp_get_default_cfg(&ctx->cfg);
  }

  if (ctx->cfg.sample_rate_hz == 0u) {
    return IMPACT_DISP_ERR_ARG;
  }

  if (ctx->cfg.max_event_ms < ctx->cfg.min_event_ms) {
    return IMPACT_DISP_ERR_ARG;
  }

  if (ctx->cfg.release_count_min == 0u) {
    ctx->cfg.release_count_min = 1u;
  }

  ctx->state = IMPACT_DISP_STATE_IDLE;
  return IMPACT_DISP_OK;
}

impact_disp_status_t impact_disp_begin_event(impact_disp_ctx_t *ctx,
                                             uint32_t event_id) {
  impact_disp_cfg_t backup_cfg;
  uint8_t baseline_set;
  float bx;
  float by;
  float bz;

  if (ctx == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }

  if (ctx->state == IMPACT_DISP_STATE_UNINIT) {
    return IMPACT_DISP_ERR_STATE;
  }

  backup_cfg = ctx->cfg;
  baseline_set = ctx->baseline_set;
  bx = ctx->baseline_ax_mg;
  by = ctx->baseline_ay_mg;
  bz = ctx->baseline_az_mg;

  memset(ctx, 0, sizeof(*ctx));
  ctx->cfg = backup_cfg;
  ctx->baseline_set = baseline_set;
  ctx->baseline_ax_mg = bx;
  ctx->baseline_ay_mg = by;
  ctx->baseline_az_mg = bz;

  /* If EMA gravity is enabled, re-initialize the estimator from the baseline
   * (streaming callers may set baseline before begin_event). */
  if (ctx->cfg.gravity_ema_tau_ms != 0u && ctx->baseline_set != 0u) {
    ctx->g_hat_ax_mg = ctx->baseline_ax_mg;
    ctx->g_hat_ay_mg = ctx->baseline_ay_mg;
    ctx->g_hat_az_mg = ctx->baseline_az_mg;
    ctx->g_hat_set = 1u;
  }

  ctx->event_id = event_id;
  ctx->state = IMPACT_DISP_STATE_COLLECTING;
  return IMPACT_DISP_OK;
}

impact_disp_status_t impact_disp_set_baseline_mg(impact_disp_ctx_t *ctx,
                                                 float bx_mg, float by_mg,
                                                 float bz_mg) {
  if (ctx == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }
  if (ctx->state == IMPACT_DISP_STATE_UNINIT) {
    return IMPACT_DISP_ERR_STATE;
  }

  ctx->baseline_ax_mg = bx_mg;
  ctx->baseline_ay_mg = by_mg;
  ctx->baseline_az_mg = bz_mg;
  ctx->baseline_set = 1u;

  if (ctx->cfg.gravity_ema_tau_ms != 0u) {
    ctx->g_hat_ax_mg = bx_mg;
    ctx->g_hat_ay_mg = by_mg;
    ctx->g_hat_az_mg = bz_mg;
    ctx->g_hat_set = 1u;
  }
  return IMPACT_DISP_OK;
}

impact_disp_status_t impact_disp_feed_sample(impact_disp_ctx_t *ctx,
                                             const impact_disp_sample_t *sample) {
  float ax_dyn_mg;
  float ay_dyn_mg;
  float az_dyn_mg;
  float a_mag_dyn_mg;
  float a_curr_x;
  float a_curr_y;
  float a_curr_z;
  float ref_ax_mg;
  float ref_ay_mg;
  float ref_az_mg;
  uint16_t grav_tau_ms;

  if (ctx == NULL || sample == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }
  if (ctx->state != IMPACT_DISP_STATE_COLLECTING) {
    return IMPACT_DISP_ERR_STATE;
  }

  grav_tau_ms = ctx->cfg.gravity_ema_tau_ms;

  if (grav_tau_ms != 0u) {
    if (ctx->g_hat_set == 0u) {
      /* If caller didn't set a baseline, initialize from the first sample. */
      if (ctx->baseline_set != 0u) {
        ctx->g_hat_ax_mg = ctx->baseline_ax_mg;
        ctx->g_hat_ay_mg = ctx->baseline_ay_mg;
        ctx->g_hat_az_mg = ctx->baseline_az_mg;
      } else {
        ctx->g_hat_ax_mg = (float)sample->ax_mg;
        ctx->g_hat_ay_mg = (float)sample->ay_mg;
        ctx->g_hat_az_mg = (float)sample->az_mg;
      }
      ctx->g_hat_set = 1u;
    }

    /* Use previous estimate as reference (high-pass behavior). */
    ref_ax_mg = ctx->g_hat_ax_mg;
    ref_ay_mg = ctx->g_hat_ay_mg;
    ref_az_mg = ctx->g_hat_az_mg;
  } else {
    ref_ax_mg = ctx->baseline_ax_mg;
    ref_ay_mg = ctx->baseline_ay_mg;
    ref_az_mg = ctx->baseline_az_mg;
  }

  ax_dyn_mg = (float)sample->ax_mg - ref_ax_mg;
  ay_dyn_mg = (float)sample->ay_mg - ref_ay_mg;
  az_dyn_mg = (float)sample->az_mg - ref_az_mg;
  a_mag_dyn_mg = impact_disp_norm3(ax_dyn_mg, ay_dyn_mg, az_dyn_mg);

  if (a_mag_dyn_mg > ctx->peak_acc_mg) {
    ctx->peak_acc_mg = a_mag_dyn_mg;
  }

  if ((sample->flags & IMPACT_DISP_SAMPLE_CLIPPED) != 0u) {
    ctx->quality_flags |= IMPACT_DISP_QF_CLIPPED;
  }

  if (a_mag_dyn_mg <= (float)ctx->cfg.release_threshold_mg) {
    if (ctx->release_count < 0xFFFFu) {
      ctx->release_count++;
    }
    ctx->tail_sum_ax_mg += (float)sample->ax_mg;
    ctx->tail_sum_ay_mg += (float)sample->ay_mg;
    ctx->tail_sum_az_mg += (float)sample->az_mg;
    ctx->tail_count++;
  } else {
    ctx->release_count = 0u;
  }

  a_curr_x = ax_dyn_mg * MG_TO_MPS2;
  a_curr_y = ay_dyn_mg * MG_TO_MPS2;
  a_curr_z = az_dyn_mg * MG_TO_MPS2;

  if (ctx->has_prev == 0u) {
    ctx->first_ts_us = sample->timestamp_us;
    ctx->last_ts_us = sample->timestamp_us;
    ctx->prev_ax_mps2 = a_curr_x;
    ctx->prev_ay_mps2 = a_curr_y;
    ctx->prev_az_mps2 = a_curr_z;
    ctx->has_prev = 1u;
    ctx->sample_count = 1u;
    return IMPACT_DISP_OK;
  }

  {
    uint32_t delta_us;
    float dt_s;
    float vx_new;
    float vy_new;
    float vz_new;

    if (sample->timestamp_us <= ctx->last_ts_us) {
      ctx->quality_flags |= IMPACT_DISP_QF_TS_NON_MONOTONIC;
      delta_us = impact_disp_nominal_dt_us(&ctx->cfg);
      if (delta_us == 0u) {
        delta_us = 1u;
      }
    } else {
      delta_us = sample->timestamp_us - ctx->last_ts_us;
    }

    if (delta_us > ((uint32_t)ctx->cfg.max_dt_ms * 1000u)) {
      ctx->quality_flags |= IMPACT_DISP_QF_DT_GAP;
    }

    dt_s = (float)delta_us / 1000000.0f;

    if (grav_tau_ms != 0u && ctx->g_hat_set != 0u) {
      float tau_s = (float)grav_tau_ms / 1000.0f;
      float alpha = (tau_s > 0.0f) ? (dt_s / (tau_s + dt_s)) : 1.0f;
      if (alpha < 0.0f) {
        alpha = 0.0f;
      } else if (alpha > 1.0f) {
        alpha = 1.0f;
      }

      ctx->g_hat_ax_mg += alpha * ((float)sample->ax_mg - ctx->g_hat_ax_mg);
      ctx->g_hat_ay_mg += alpha * ((float)sample->ay_mg - ctx->g_hat_ay_mg);
      ctx->g_hat_az_mg += alpha * ((float)sample->az_mg - ctx->g_hat_az_mg);
    }

    vx_new = ctx->vx_mps + 0.5f * (ctx->prev_ax_mps2 + a_curr_x) * dt_s;
    vy_new = ctx->vy_mps + 0.5f * (ctx->prev_ay_mps2 + a_curr_y) * dt_s;
    vz_new = ctx->vz_mps + 0.5f * (ctx->prev_az_mps2 + a_curr_z) * dt_s;

    ctx->x_m += 0.5f * (ctx->vx_mps + vx_new) * dt_s;
    ctx->y_m += 0.5f * (ctx->vy_mps + vy_new) * dt_s;
    ctx->z_m += 0.5f * (ctx->vz_mps + vz_new) * dt_s;

    ctx->vx_mps = vx_new;
    ctx->vy_mps = vy_new;
    ctx->vz_mps = vz_new;
  }

  ctx->prev_ax_mps2 = a_curr_x;
  ctx->prev_ay_mps2 = a_curr_y;
  ctx->prev_az_mps2 = a_curr_z;
  ctx->last_ts_us = sample->timestamp_us;
  ctx->sample_count++;

  if ((ctx->last_ts_us - ctx->first_ts_us) >
      ((uint32_t)ctx->cfg.max_event_ms * 1000u)) {
    ctx->state = IMPACT_DISP_STATE_FINISHED;
  }

  return IMPACT_DISP_OK;
}

impact_disp_status_t impact_disp_end_event(impact_disp_ctx_t *ctx,
                                           impact_disp_result_t *out) {
  float duration_s;
  float rot_err_mg = 0.0f;
  float dx = 0.0f;
  float dy = 0.0f;
  float dz = 0.0f;
  float vterm_x = 0.0f;
  float vterm_y = 0.0f;
  float vterm_z = 0.0f;

  if (ctx == NULL || out == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }
  if (ctx->state != IMPACT_DISP_STATE_COLLECTING &&
      ctx->state != IMPACT_DISP_STATE_FINISHED) {
    return IMPACT_DISP_ERR_STATE;
  }
  if (ctx->sample_count < 2u) {
    ctx->quality_flags |= IMPACT_DISP_QF_LOW_SAMPLE_COUNT;
    return IMPACT_DISP_ERR_NOT_READY;
  }

  memset(out, 0, sizeof(*out));

  duration_s = (float)(ctx->last_ts_us - ctx->first_ts_us) / 1000000.0f;
  if (duration_s <= 0.0f) {
    return IMPACT_DISP_ERR_NOT_READY;
  }

  if ((ctx->last_ts_us - ctx->first_ts_us) <
      ((uint32_t)ctx->cfg.min_event_ms * 1000u)) {
    ctx->quality_flags |= IMPACT_DISP_QF_DURATION_TOO_SHORT;
  }

  if (ctx->release_count < ctx->cfg.release_count_min) {
    ctx->quality_flags |= IMPACT_DISP_QF_NO_RELEASE;
  }

  if (ctx->baseline_set != 0u && ctx->tail_count > 0u) {
    float g_end_x = ctx->tail_sum_ax_mg / (float)ctx->tail_count;
    float g_end_y = ctx->tail_sum_ay_mg / (float)ctx->tail_count;
    float g_end_z = ctx->tail_sum_az_mg / (float)ctx->tail_count;
    float dgx = g_end_x - ctx->baseline_ax_mg;
    float dgy = g_end_y - ctx->baseline_ay_mg;
    float dgz = g_end_z - ctx->baseline_az_mg;
    rot_err_mg = impact_disp_norm3(dgx, dgy, dgz);

    if (rot_err_mg > impact_disp_rotation_limit_mg(ctx->cfg.rotation_limit_deg)) {
      ctx->quality_flags |= IMPACT_DISP_QF_ROTATION_HIGH;
    }
  }

  if (ctx->cfg.enable_zero_velocity_correction != 0u) {
    float b_res_x = ctx->vx_mps / duration_s;
    float b_res_y = ctx->vy_mps / duration_s;
    float b_res_z = ctx->vz_mps / duration_s;
    float t2 = duration_s * duration_s;

    dx = ctx->x_m - 0.5f * b_res_x * t2;
    dy = ctx->y_m - 0.5f * b_res_y * t2;
    dz = ctx->z_m - 0.5f * b_res_z * t2;

    vterm_x = ctx->vx_mps - b_res_x * duration_s;
    vterm_y = ctx->vy_mps - b_res_y * duration_s;
    vterm_z = ctx->vz_mps - b_res_z * duration_s;
  } else {
    dx = ctx->x_m;
    dy = ctx->y_m;
    dz = ctx->z_m;
    vterm_x = ctx->vx_mps;
    vterm_y = ctx->vy_mps;
    vterm_z = ctx->vz_mps;
  }

  out->event_id = ctx->event_id;
  out->duration_ms = (uint32_t)(duration_s * 1000.0f);
  out->sample_count = ctx->sample_count;
  out->dx_mm = dx * M_TO_MM;
  out->dy_mm = dy * M_TO_MM;
  out->dz_mm = dz * M_TO_MM;
  out->disp_mm = impact_disp_norm3(out->dx_mm, out->dy_mm, out->dz_mm);
  out->peak_acc_mg = ctx->peak_acc_mg;
  out->terminal_speed_mm_s =
      impact_disp_norm3(vterm_x, vterm_y, vterm_z) * MPS_TO_MMPS;
  out->rotation_error_mg = rot_err_mg;
  out->quality_flags = ctx->quality_flags;
  out->confidence = impact_disp_clamp_u8(impact_disp_confidence_from_flags(
      out->quality_flags, out->sample_count, ctx->release_count,
      ctx->cfg.release_count_min));

  ctx->state = IMPACT_DISP_STATE_FINISHED;
  return IMPACT_DISP_OK;
}

impact_disp_status_t impact_disp_process_event(impact_disp_ctx_t *ctx,
                                               const impact_disp_sample_t *samples,
                                               uint32_t sample_count,
                                               uint16_t pre_samples,
                                               uint32_t event_id,
                                               impact_disp_result_t *out) {
  uint32_t i;
  float sum_ax = 0.0f;
  float sum_ay = 0.0f;
  float sum_az = 0.0f;
  impact_disp_status_t st;

  if (ctx == NULL || samples == NULL || out == NULL) {
    return IMPACT_DISP_ERR_ARG;
  }
  if (sample_count < 2u) {
    return IMPACT_DISP_ERR_ARG;
  }
  if (pre_samples == 0u || pre_samples > sample_count) {
    return IMPACT_DISP_ERR_ARG;
  }

  for (i = 0u; i < pre_samples; ++i) {
    sum_ax += (float)samples[i].ax_mg;
    sum_ay += (float)samples[i].ay_mg;
    sum_az += (float)samples[i].az_mg;
  }

  st = impact_disp_begin_event(ctx, event_id);
  if (st != IMPACT_DISP_OK) {
    return st;
  }

  st = impact_disp_set_baseline_mg(ctx, sum_ax / (float)pre_samples,
                                   sum_ay / (float)pre_samples,
                                   sum_az / (float)pre_samples);
  if (st != IMPACT_DISP_OK) {
    return st;
  }

  for (i = 0u; i < sample_count; ++i) {
    st = impact_disp_feed_sample(ctx, &samples[i]);
    if (st != IMPACT_DISP_OK) {
      return st;
    }
  }

  return impact_disp_end_event(ctx, out);
}

impact_disp_state_t impact_disp_get_state(const impact_disp_ctx_t *ctx) {
  if (ctx == NULL) {
    return IMPACT_DISP_STATE_ERROR;
  }
  return ctx->state;
}
