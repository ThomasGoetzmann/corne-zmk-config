/*
 * Fixed-color RGB underglow pulse for mode indication.
 * On key press: sets underglow to the hue given by param1 (0-359),
 * with full saturation and brightness, then schedules a delayed work
 * item to turn it off after pulse-ms milliseconds.
 */

#define DT_DRV_COMPAT zmk_behavior_mode_rgb

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_mode_rgb_config {
    uint16_t pulse_ms;
};

static struct k_work_delayable mode_rgb_off_work;

static void mode_rgb_off_cb(struct k_work *work) { zmk_rgb_underglow_off(); }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_mode_rgb_config *cfg = dev->config;

    struct zmk_led_hsb color = {
        .h = binding->param1,
        .s = 100,
        .b = 100,
    };

    zmk_rgb_underglow_select_effect(0); /* force solid */
    zmk_rgb_underglow_on();
    zmk_rgb_underglow_set_hsb(color);
    k_work_reschedule(&mode_rgb_off_work, K_MSEC(cfg->pulse_ms));

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_mode_rgb_init(const struct device *dev) {
    static bool initialized = false;
    if (!initialized) {
        k_work_init_delayable(&mode_rgb_off_work, mode_rgb_off_cb);
        initialized = true;
    }
    return 0;
}

static const struct behavior_driver_api behavior_mode_rgb_driver_api = {
    .binding_pressed  = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality         = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define MODE_RGB_INST(n)                                                                       \
    static const struct behavior_mode_rgb_config behavior_mode_rgb_config_##n = {             \
        .pulse_ms = DT_INST_PROP(n, pulse_ms),                                                \
    };                                                                                         \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_mode_rgb_init, NULL, NULL,                            \
                            &behavior_mode_rgb_config_##n,                                     \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                            &behavior_mode_rgb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MODE_RGB_INST)
