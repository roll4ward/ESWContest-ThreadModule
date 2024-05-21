cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(NetworkModule)

target_sources(app PRIVATE
                    src/main.c
                    src/thread_net_config.c
                )

target_sources_ifdef(CONFIG_SHELL app PRIVATE src/thread_net_config_shell.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/bluetooth_ad.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/bluetooth_conn.c)
target_sources_ifdef(CONFIG_BT app PRIVATE src/bluetooth_button.c)
target_sources_ifdef(CONFIG_GPIO app PRIVATE src/action_button.c)
target_sources_ifdef(CONFIG_GPIO app PRIVATE src/button_service.c)

include_directories(include)