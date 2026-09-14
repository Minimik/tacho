cmake_minimum_required(VERSION 3.16)

set(PARTITION_TABLE_CUSTOM_FILENAME
    "${CMAKE_CURRENT_LIST_DIR}/partitions.csv"
    CACHE FILEPATH
    "Custom partition table CSV file"
    FORCE
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(tacho)