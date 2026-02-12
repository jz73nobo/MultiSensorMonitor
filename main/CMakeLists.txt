idf_component_register(SRCS "main.c" "sensor_sound.c" "sensor_vibe.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver esp-dsp)
