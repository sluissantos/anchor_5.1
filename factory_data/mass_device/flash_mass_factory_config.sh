#!/bin/sh
sudo ~/.espressif/python_env/idf5.1_py3.10_env/bin/python3 ${IDF_PATH}/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate mass_factory_conf.csv 0x3000