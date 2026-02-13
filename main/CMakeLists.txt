idf_component_register(SRCS "sound.c" "main.c" "sensor_vibe.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver esp_timer esp-dsp)
