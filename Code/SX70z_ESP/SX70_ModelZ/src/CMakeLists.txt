# 外设/控制代码组件
idf_component_register(SRCS "devinfo.c" "camera_main.c" "ota_web.c" "pin_init.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_server app_update driver)
