idf_component_register(SRCS board_cfg.c ds1307.c
                        PRIV_INCLUDE_DIRS .
                        PRIV_REQUIRES esp_driver_i2c nvs_flash
                     ) 
