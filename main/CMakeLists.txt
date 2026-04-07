idf_component_register(SRCS actuator.c board_cfg.c clearButton.c comm.c ds1307.c fota.c heartBeat.c main.c nvCron.c nvLogRing.c stateProbe.c tmu.c trap.c
                        PRIV_INCLUDE_DIRS .
                        PRIV_REQUIRES nvs_flash esp_event esp_wifi esp_https_ota esp_driver_gpio esp_driver_i2c freertos esp_timer) 
