# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "extra_components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(warehouse_anchor_esp)
target_add_binary_data(warehouse_anchor_esp.elf "certificates/mqtt_broker/globalsign_root_ca_r2.pem" TEXT)
target_add_binary_data(warehouse_anchor_esp.elf "certificates/firmware_upgrade/root_ca.pem" TEXT)

idf_build_set_property(COMPILE_OPTIONS -DCONFIG_PARTITION_TABLE_SINGLE_APP_LARGE=1 APPEND)
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_BT_ENABLED=1 APPEND)
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_BT_GATTC_ENABLE=0 APPEND)
# Adicione outras opções conforme necessário

# Configurações personalizadas
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_DEFAULT_FIRMWARE_UPGRADE_URI=\"https://url:port/path\" APPEND)
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_DEFAULT_FIRMWARE_UPGRADE_AUTH_USER=\"tecnologia\" APPEND)
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_DEFAULT_FIRMWARE_UPGRADE_AUTH_PASSWORD=\"128Parsecs!\" APPEND)
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_DEFAULT_FIRMWARE_UPGRADE_ATTEMPTS=3 APPEND)
idf_build_set_property(COMPILE_OPTIONS -DCONFIG_DEFAULT_FIRMWARE_UPGRADE_TIMEOUT=120 APPEND)
