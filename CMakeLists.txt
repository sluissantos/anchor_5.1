# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "extra_components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(warehouse_anchor_esp)
target_add_binary_data(warehouse_anchor_esp.elf "certificates/mqtt_broker/globalsign_root_ca_r2.pem" TEXT)
target_add_binary_data(warehouse_anchor_esp.elf "certificates/firmware_upgrade/root_ca.pem" TEXT)
