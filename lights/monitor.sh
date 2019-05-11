#!/usr/bin/env bash

python2 $IDF_PATH/tools/idf_monitor.py .pioenvs/esp32/firmware.elf "$@"
