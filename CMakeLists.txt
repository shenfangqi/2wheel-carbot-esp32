cmake_minimum_required(VERSION 3.16)

# Build only the application and its declared dependency closure. This keeps
# unused Wi-Fi, network-stack, and HTTP components out of serial-only firmware.
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(carbot)
