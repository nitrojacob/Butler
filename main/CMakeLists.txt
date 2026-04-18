idf_component_register(SRCS actuator.c clearButton.c comm.c fota.c heartBeat.c main.c nvCron.c nvLogRing.c stateProbe.c sysStat.c tmu.c trap.c
                        PRIV_INCLUDE_DIRS . ../chip_esp32
                        PRIV_REQUIRES nvs_flash esp_event esp_wifi esp_https_ota esp_driver_gpio freertos esp_timer chip_esp32
                        EMBED_TXTFILES ../server_certs/ca_cert.pem)
