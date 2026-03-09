/*
 * Battery-dependent RGB underglow pulse behavior.
 * On key press: sets underglow color based on battery level, turns it on,
 * then schedules a delayed work item to turn it off after pulse-ms.
 *
 *   green  → battery > high_threshold (default 40%)
 *   yellow → battery > low_threshold  (default 20%)
 *   red    → battery <= low_threshold
 */

#define DT_DRV_COMPAT zmk_behavior_bat_rgb

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/battery.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_bat_rgb_config {
    uint8_t high_threshold;
    uint8_t low_threshold;
    uint16_t pulse_ms;
};

static struct k_work_delayable bat_rgb_off_work;

static void bat_rgb_off_cb(struct k_work *work) { zmk_rgb_underglow_off(); }

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

    zmk_rgb_underglow_set_hsb(color);
    zmk_rgb_underglow_on();
    k_work_reschedule(&bat_rgb_off_work, K_MSEC(cfg->pulse_ms));

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_bat_rgb_init(const struct device *dev) {
    static bool initialized = false;
    if (!initialized) {
        k_work_init_delayable(&bat_rgb_off_work, bat_rgb_off_cb);
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
    static const struct behavior_bat_rgb_config behavior_bat_rgb_config_##n = {               \
        .high_threshold = DT_INST_PROP(n, high_threshold),                                    \
        .low_threshold  = DT_INST_PROP(n, low_threshold),                                     \
        .pulse_ms       = DT_INST_PROP(n, pulse_ms),                                          \
    };                                                                                         \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_bat_rgb_init, NULL, NULL,                             \
                            &behavior_bat_rgb_config_##n,                                      \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                            &behavior_bat_rgb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BAT_RGB_INST)
