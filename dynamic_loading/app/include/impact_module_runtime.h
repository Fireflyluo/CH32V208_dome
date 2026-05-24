#ifndef IMPACT_MODULE_RUNTIME_H
#define IMPACT_MODULE_RUNTIME_H

#include "impact_displacement.h"
#include "impact_module_api.h"

#include <stdbool.h>
#include <stdint.h>

bool impact_module_runtime_init(void);
bool impact_module_runtime_ready(void);
int32_t impact_module_runtime_get_default_cfg(impact_disp_cfg_t *cfg);
int32_t impact_module_runtime_configure(const impact_disp_cfg_t *cfg);
int32_t impact_module_runtime_reset(void);
int32_t impact_module_runtime_begin_event(uint32_t event_id);
int32_t impact_module_runtime_set_baseline(float bx_mg, float by_mg, float bz_mg);
int32_t impact_module_runtime_feed_sample(const impact_disp_sample_t *sample);
int32_t impact_module_runtime_end_event(impact_module_end_response_t *response);
int32_t impact_module_runtime_get_diag(impact_module_diag_t *diag);

#endif /* IMPACT_MODULE_RUNTIME_H */
