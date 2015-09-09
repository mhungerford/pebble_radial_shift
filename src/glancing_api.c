#include <pebble.h>
#include "glancing_api.h"

#define WITHIN(n, min, max) (((n)>=(min) && (n) <= (max)) ? true : false)

static void prv_glancing_callback(GlancingData *data) {}
static GlancingDataHandler prv_handler = prv_glancing_callback;
static GlancingData prv_data = {.state = GLANCING_INACTIVE};
static int prv_timeout_ms = 0;
static AppTimer *glancing_timeout_handle = NULL;
static bool prv_control_backlight = false;
static GlanceState glance_state = GLANCING_INACTIVE;

static inline void prv_update_state(GlanceState state) {
  if (prv_data.state != state) {
    prv_data.state = state;
    prv_handler(&prv_data);
  }
}

static inline bool prv_is_glancing(void) {
  return prv_data.state == GLANCING_ACTIVE;
}

static void glance_timeout(void* data) {
  prv_update_state(GLANCING_TIMEDOUT);
}

// Light interactive timer to save power by not turning on light in ambient sunlight
static void prv_light_timer(void *data) {
  if (prv_is_glancing()) {
    app_timer_register(2 * 1000, prv_light_timer, data);
    light_enable_interaction();
  }
}

static void prv_accel_handler(AccelData *data, uint32_t num_samples) {
  static bool unglanced = true;
  for (uint32_t i = 0; i < num_samples; i++) {
    if(
        WITHIN(data[i].x, -400, 400) &&
        WITHIN(data[i].y, -900, 0) &&
        WITHIN(data[i].z, -1100, -300)) {
      if (unglanced) {
        unglanced = false;
        prv_update_state(GLANCING_ACTIVE);
        // timeout for glancing
        if (prv_timeout_ms) {
          glancing_timeout_handle = app_timer_register(prv_timeout_ms, glance_timeout, data);
        }
        if (prv_control_backlight) {
          prv_light_timer(NULL);
        }
      }
      return;
    }
  }
  // Disable timeout if unnecessary
  if (glancing_timeout_handle) {
    app_timer_cancel(glancing_timeout_handle);
  }
  unglanced = true;
  prv_update_state(GLANCING_INACTIVE);
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction){
  if(!prv_is_glancing()) {
    // force light to be off when we are not looking
    // ie. save power if accidental flick angles
    light_enable(false);
  }
}

void glancing_service_subscribe(
    int timeout_ms, bool control_backlight, GlancingDataHandler handler) {
  prv_handler = handler;
  prv_timeout_ms = (timeout_ms > 0) ? timeout_ms : 0;

  //Setup motion accel handler with low sample rate
  accel_data_service_subscribe(5, prv_accel_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  
  prv_control_backlight = control_backlight;
  if (prv_control_backlight) {
    //Setup tap service to avoid old flick to light behavior
    accel_tap_service_subscribe(prv_tap_handler);
  }
}
