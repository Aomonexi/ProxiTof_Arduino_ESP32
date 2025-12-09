# ProxiTof Near-Miss Detection System
# ESP-IDF CMake Project

cmake_minimum_required(VERSION 3.16)

# Set extra component directories
set(EXTRA_COMPONENT_DIRS components)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(proxitof)
