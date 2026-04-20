if(CONFIG_ZMK_BEHAVIOR_BAT_RGB)
  zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
  zephyr_library_sources(src/behaviors/behavior_bat_rgb.c)
endif()

if(CONFIG_ZMK_BEHAVIOR_MODE_RGB)
  zephyr_library_sources(src/behaviors/behavior_mode_rgb.c)
endif()
