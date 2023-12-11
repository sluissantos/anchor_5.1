#!/bin/sh
#build, merge and flash
idf.py build
cd factory_data/single_device/ && bash flash_single_factory_data.sh && cd ./../.. 
~/.espressif/python_env/idf5.1_py3.10_env/bin/python ~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip ESP32 merge_bin -o build/combined_esp32.bin --flash_mode dio --flash_size 4MB 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x9000 build/single_factory_data.bin 0xd000 build/ota_data_initial.bin 0xf000 build/phy_init_data.bin 0x10000 build/warehouse_anchor_esp.bin 
~/.espressif/python_env/idf5.1_py3.10_env/bin/python ~/esp/esp-idf/components/esptool_py/esptool/esptool.py -b 1500000 --before default_reset --after hard_reset --chip esp32 write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m 0x0 build/combined_esp32.bin
idf.py monitor