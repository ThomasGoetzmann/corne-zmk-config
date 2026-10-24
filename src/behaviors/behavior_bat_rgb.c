/*
 * Battery + output RGB underglow indicator.
 * On key press, plays a short sequence on the underglow, then turns it off:
 *
 *   1. battery level for pulse-ms (both halves, each shows its own battery)
 *        green  → battery > high_threshold (default 40%)
 *        yellow → battery > low_threshold  (default 20%)
 *        red    → battery <= low_threshold
 *   2. dark for gap-ms                       (central only)
 *   3. active output for profile-pulse-ms    (central only)
 *        USB → solid white
 *        BLE → hue from profile-hues[active profile], pattern from its status:
 *              connected       → solid
 *              paired, offline → double blink
 *              open (unpaired) → fast flicker
 *
 * The peripheral half has no BLE profile/endpoint state, so it stays dark
 * after phase 1. Pressing again restarts the sequence; a mode_rgb pulse
 * cancels it via zmk_bat_rgb_cancel().
 */

#define DT_DRV_COMPAT zmk_behavior_bat_rgb

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/battery.h>
#include <zmk/rgb_underglow.h>

#define BAT_RGB_SHOWS_OUTPUT                                                                   \
    (IS_ENABLED(CONFIG_ZMK_BLE) &&                                                             \
     (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)))

#if BAT_RGB_SHOWS_OUTPUT
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_bat_rgb_config {
    uint8_t high_threshold;
    uint8_t low_threshold;
    uint16_t pulse_ms;
    uint16_t gap_ms;
    uint16_t profile_pulse_ms;
    const uint16_t *profile_hues;
    size_t profile_hue_count;
};

/* One underglow state held for a duration; color is ignored when off. */
struct bat_rgb_step {
    bool on;
    struct zmk_led_hsb color;
    uint16_t ms;
};

#define BAT_RGB_MAX_STEPS 16

static struct bat_rgb_step steps[BAT_RGB_MAX_STEPS];
static uint8_t step_count;
static uint8_t step_idx;

static struct k_work_delayable bat_rgb_work;

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_MODE_RGB)
void zmk_mode_rgb_cancel(void);
#endif

static void add_step(bool on, struct zmk_led_hsb color, uint16_t ms) {
    if (step_count < BAT_RGB_MAX_STEPS) {
        steps[step_count++] = (struct bat_rgb_step){.on = on, .color = color, .ms = ms};
    }
}

static void run_next_step(void) {
    if (step_idx >= step_count) {
        zmk_rgb_underglow_off();
        return;
    }

    const struct bat_rgb_step *step = &steps[step_idx++];
    if (step->on) {
        zmk_rgb_underglow_on();
        zmk_rgb_underglow_set_hsb(step->color);
    } else {
        zmk_rgb_underglow_off();
    }
    k_work_reschedule(&bat_rgb_work, K_MSEC(step->ms));
}

static void bat_rgb_work_cb(struct k_work *work) { run_next_step(); }

void zmk_bat_rgb_cancel(void) {
    k_work_cancel_delayable(&bat_rgb_work);
    step_count = 0;
    step_idx = 0;
}

#if BAT_RGB_SHOWS_OUTPUT
static void add_output_steps(const struct behavior_bat_rgb_config *cfg) {
    const struct zmk_led_hsb off = {0};
    const uint16_t p = cfg->profile_pulse_ms;

    if (zmk_endpoint_get_selected().transport == ZMK_TRANSPORT_USB) {
        add_step(true, (struct zmk_led_hsb){.h = 0, .s = 0, .b = 100}, p); /* white */
        return;
    }

    uint8_t profile = zmk_ble_active_profile_index();
    struct zmk_led_hsb color = {
        .h = cfg->profile_hues[profile % cfg->profile_hue_count],
        .s = 100,
        .b = 100,
    };

    if (zmk_ble_active_profile_is_connected()) {
        add_step(true, color, p);
    } else if (zmk_ble_active_profile_is_open()) {
        for (int i = 0; i < 5; i++) {
            add_step(true, color, p / 10);
            add_step(false, off, p / 10);
        }
    } else {
        add_step(true, color, p * 3 / 10);
        add_step(false, off, p / 5);
        add_step(true, color, p * 3 / 10);
    }
}
#endif

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_bat_rgb_config *cfg = dev->config;

    uint8_t level = zmk_battery_state_of_charge();
    struct zmk_led_hsb color;

    if (level > cfg->high_threshold) {
        color = (struct zmk_led_hsb){.h = 120, .s = 100, .b = 100}; /* green  */
    } else if (level > cfg->low_threshold) {
        color = (struct zmk_led_hsb){.h = 60, .s = 100, .b = 100};  /* yellow */
    } else {
        color = (struct zmk_led_hsb){.h = 0, .s = 100, .b = 100};   /* red    */
    }

    zmk_bat_rgb_cancel();
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_MODE_RGB)
    zmk_mode_rgb_cancel();
#endif
    add_step(true, color, cfg->pulse_ms);
#if BAT_RGB_SHOWS_OUTPUT
    add_step(false, (struct zmk_led_hsb){0}, cfg->gap_ms);
    add_output_steps(cfg);
#endif

    zmk_rgb_underglow_select_effect(0); /* force solid */
    run_next_step();

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_bat_rgb_init(const struct device *dev) {
    static bool initialized = false;
    if (!initialized) {
        k_work_init_delayable(&bat_rgb_work, bat_rgb_work_cb);
        initialized = true;
    }
    return 0;
}

static const struct behavior_driver_api behavior_bat_rgb_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define BAT_RGB_INST(n)                                                                        \
    static const uint16_t behavior_bat_rgb_hues_##n[] = DT_INST_PROP(n, profile_hues);        \
    static const struct behavior_bat_rgb_config behavior_bat_rgb_config_##n = {               \
        .high_threshold    = DT_INST_PROP(n, high_threshold),                                 \
        .low_threshold     = DT_INST_PROP(n, low_threshold),                                  \
        .pulse_ms          = DT_INST_PROP(n, pulse_ms),                                       \
        .gap_ms            = DT_INST_PROP(n, gap_ms),                                         \
        .profile_pulse_ms  = DT_INST_PROP(n, profile_pulse_ms),                               \
        .profile_hues      = behavior_bat_rgb_hues_##n,                                       \
        .profile_hue_count = ARRAY_SIZE(behavior_bat_rgb_hues_##n),                           \
    };                                                                                         \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_bat_rgb_init, NULL, NULL,                             \
                            &behavior_bat_rgb_config_##n,                                      \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                            &behavior_bat_rgb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BAT_RGB_INST)
