cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ESWContestThreadModule)

target_sources(app PRIVATE
                    src/main.c
                )

target_sources_ifdef(CONFIG_BT app PRIVATE src/bluetooth_ad.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/ble_commission.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/ble_common.c)
target_sources_ifdef(CONFIG_OPENTHREAD_COAP app PRIVATE src/coap.c)
target_sources_ifdef(CONFIG_GPIO app PRIVATE src/action_button.c)
target_sources_ifdef(CONFIG_GPIO app PRIVATE src/button_service.c)
target_sources_ifdef(CONFIG_GPIO app PRIVATE src/reset_button.c)

include_directories(include)