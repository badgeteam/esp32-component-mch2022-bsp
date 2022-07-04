idf_component_register(
    SRCS "hardware.c"
         "wifi_connection.c"
         "wifi_connect.c"
    INCLUDE_DIRS "." "include"
    REQUIRES
        "spi-ice40"
        "mch2022-rp2040"
        "wpa_supplicant"
        "nvs_flash"
)
